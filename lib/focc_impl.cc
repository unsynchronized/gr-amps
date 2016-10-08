/* Written by Brandon Creighton <cstone@pobox.com>.
 *
 * This code is in the public domain; however, note that most of its
 * dependent code, including GNU Radio, is not.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "focc_impl.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <vector>
#include "utils.h"

using namespace itpp;
using std::vector;
using std::string;
using std::cout;
using std::endl;
using boost::shared_ptr;

namespace gr {
    namespace amps {

        volatile bool busy_idle_bit;


        /**
         * AMPS BS FOCC.  553 3.7.1.2 says that the system parameter overhead 
         * message needs to be sent every 0.8 +/- 0.3 s.
         *
         * The BS simulator is repeating the overhead words every 18 or 19 
         * frames, putting this squarely in the range:
         *     18 x 463 bits = 8334 bits (0.8334s)
         *     19 x 463 bits = 8797 bits (0.8797s)
         *
         * This code uses 18 in a superframe, starting with word 1 and word 2.
         */

        focc::sptr
        focc::make(unsigned long symrate) {
            return gnuradio::get_initial_sptr (new focc_impl(symrate));
        }


        void
        focc_impl::queue_file() {
            std::ifstream bitfile("/tmp/out.bits");
            std::cout << "XXX queue_file:    start: sz " << queuesize() << std::endl;
            unsigned long zerocount = 0, onecount = 1;
            while(bitfile) {
                char buf[1];
                buf[0] = 0;
                if(!bitfile.read(buf, 1)) {
                    break;
                }
                if(buf[0] == 1 || buf[0] == '1') {
                    onecount++;
                    queuebit(0);
                    queuebit(1);
                } else if(buf[0] == 0 || buf[0] == '0') {
                    zerocount++;
                    queuebit(1);
                    queuebit(0);
                } else {
                    std::cout << "error: invalid value in bits file" << std::endl;
                }
            }
            std::cout << "XXX queue_file:    final: sz " << queuesize() << std::endl;
            std::cout << "XXX queue_file:    zerocount " << zerocount << " onecount " << onecount << std::endl;
        }


        void
        focc_impl::queue_dup(bvec &bvec) {
            for(unsigned int i = 0; i < bvec.size(); i++) {
                queuebit(bvec[i]);
                queuebit(bvec[i]);
            }
        }

        void 
        focc_impl::queue(shared_ptr<bvec> bvptr) {
            for(unsigned int i = 0; i < bvptr->size(); i++) {
                queuebit((*bvptr)[i]);
            }
        }
        void 
        focc_impl::queue(gr_uint32 val) {
            for(int i = 0; i < 32; i++) {
                queuebit(((val & 0x80000000) == 0x80000000) ? 1 : 0);
                val = val << 1;
            }
        }



        focc_impl::focc_impl(unsigned long symrate)
          : d_symrate(symrate), cur_burst_state(FOCC_END), cur_off(0), bch(63, 2, true),
            samples_per_sym(symrate / 20000),
          sync_block("focc",
                  io_signature::make(0, 0, 0),
                  io_signature::make(1, 1, sizeof (unsigned char)))
        {
            busy_idle_bit = 1;
            BI_zero_buf = new char[samples_per_sym * 2];
            BI_one_buf = new char[samples_per_sym * 2];
            for(int i = 0; i < samples_per_sym; i++) {
                BI_zero_buf[i] = 1;
                BI_zero_buf[samples_per_sym+i] = -1;
                BI_one_buf[i] = -1;
                BI_one_buf[samples_per_sym+i] = 1;
            }
            make_superframe();
            validate_superframe();

            message_port_register_in(pmt::mp("focc_words"));
            set_msg_handler(pmt::mp("focc_words"),
                boost::bind(&focc_impl::focc_words_message, this, _1)
            );

#ifdef AMPS_DEBUG
            std::cout << "AMPS_DEBUG is enabled!" << std::endl;
            debugfd = open("/tmp/debug.bits", O_CREAT | O_APPEND, 0755);
#endif
        }

        // Insert bits into the queue.  Here is also where we repeat a single bit
        // so that we're emitting d_symrate symbols per second.
        inline void 
        focc_impl::queuebit(bool bit) {
            const unsigned int interp = d_symrate / 20000;
            for(unsigned int i = 0; i < interp; i++) {
                d_bitqueue.push(bit);
            }
        }

        focc_impl::~focc_impl()
        {
        }

        /*
         * Convert a 28-bit vector (represented as one bit per char, values 
         * being numeric 0 and 1) to a 40-bit bch-encoded word.
         */
        std::vector<char>
        focc_impl::focc_bch(std::vector<char> inbits) {
            std::vector<char> outvec(40);
            bvec zeroes("0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
            bvec srcbvec(28);
            charv_to_bvec(inbits, srcbvec);
            bvec padded(concat(zeroes, srcbvec));
            bvec encoded = bch.encode(padded);
            bvec final = encoded(23, encoded.size()-1);
            assert(final.size() == 40);
            for(int i = 0; i < 40; i++) {
                if(final[i] == 0) {
                    outvec[i] = 0;
                } else if(final[i] == 1) {
                    outvec[i] = 1;
                } else {
                    assert(0);
                }
            }
            return outvec;
        }

        focc_frame *
        focc_impl::make_frame(std::vector<char> word_a, std::vector<char> word_b, bool ephemeral, bool filler) {
            // XXX: remove this
            const unsigned int samples_per_sym = d_symrate / 20000;
            // XXX: memory leak haha
            std::vector<focc_segment *> segments;
            std::vector<char> bch_a = focc_bch(word_a);
            std::vector<char> bch_b = focc_bch(word_b);
            segments.push_back(new focc_segment(FOCC_BI_BIT));
            char dotting[] = { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 };
            segments.push_back(new focc_segment(dotting, 10, samples_per_sym));
            segments.push_back(new focc_segment(FOCC_BI_BIT));
            char wordsync[] = { 1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 0 };
            segments.push_back(new focc_segment(wordsync, 11, samples_per_sym));
            segments.push_back(new focc_segment(FOCC_END));

            for(int i = 0; i < 5; i++) {
                segments.push_back(new focc_segment(FOCC_BI_BIT));
                segments.push_back(new focc_segment(&bch_a[0], 10, samples_per_sym));
                segments.push_back(new focc_segment(FOCC_BI_BIT));
                segments.push_back(new focc_segment(&bch_a[10], 10, samples_per_sym));
                segments.push_back(new focc_segment(FOCC_END));
                segments.push_back(new focc_segment(FOCC_BI_BIT));
                segments.push_back(new focc_segment(&bch_a[20], 10, samples_per_sym));
                segments.push_back(new focc_segment(FOCC_BI_BIT));
                segments.push_back(new focc_segment(&bch_a[30], 10, samples_per_sym));
                segments.push_back(new focc_segment(FOCC_END));

                segments.push_back(new focc_segment(FOCC_BI_BIT));
                segments.push_back(new focc_segment(&bch_b[0], 10, samples_per_sym));
                segments.push_back(new focc_segment(FOCC_BI_BIT));
                segments.push_back(new focc_segment(&bch_b[10], 10, samples_per_sym));
                segments.push_back(new focc_segment(FOCC_END));
                segments.push_back(new focc_segment(FOCC_BI_BIT));
                segments.push_back(new focc_segment(&bch_b[20], 10, samples_per_sym));
                segments.push_back(new focc_segment(FOCC_BI_BIT));
                segments.push_back(new focc_segment(&bch_b[30], 10, samples_per_sym));
                segments.push_back(new focc_segment(FOCC_END));
            }

            return new focc_frame(segments, ephemeral, filler);
        }

        void focc_impl::validate_superframe() {
            // XXX: remove this
            const unsigned int samples_per_sym = d_symrate / 20000;     // XXX factor this out into the class or something
            unsigned long totalsyms = 0;
            for(unsigned int i = 0; i < superframe_frames.size(); i++) {
                focc_frame *frame = superframe_frames[i];
                for(unsigned int j = 0; j < frame->segments.size(); j++) {
                    focc_segment *segment = frame->segments[j];
                    switch(segment->state) {
                        case FOCC_MESSAGE:
                            assert(segment->length > 0);
                            totalsyms += segment->length;
                            break;
                        case FOCC_BI_BIT:
                            assert(segment->length == 0);
                            totalsyms += (samples_per_sym * 2);
                            break;
                        case FOCC_END:
                            assert(segment->length == 0);
                            break;
                    }

                }
            }
            // XXX DO THIS
            const unsigned int totalbits = totalsyms / (samples_per_sym * 2);     // XXX factor this out into the class or something
            assert(totalbits == (superframe_frames.size() * 463));
            std::cerr << "validate: totalsyms " << totalsyms << " totalbits " << totalbits << std::endl;
        }
        std::vector<char> overhead_word_1() {
            // NAWC = 0001
            //return string_to_cvec("1 1 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 1 0 1 0 0 0 1 1 1 0");
            // NAWC = 0010
            return string_to_cvec("1 1 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 1 0 1 0 0 1 0 1 1 0");
        }
        std::vector<char> overhead_word_2() {
            // END = 1
            //return string_to_cvec("1 1 0 0 1 1 1 1 0 0 1 0 1 0 0 1 1 0 0 1 0 1 0 0 1 1 1 1");

            // END = 0
            return string_to_cvec("1 1 0 0 1 1 1 1 0 0 1 0 1 0 0 1 1 0 0 1 0 1 0 0 0 1 1 1");
        }
        std::vector<char> control_filler_word() {
            return string_to_cvec("1 1 0 0 0 1 0 1 1 1 0 1 1 0 0 1 1 0 0 1 1 1 1 1 1 0 0 1");
        }
        std::vector<char> access_type_parameters_global_action() {
            // DCC = 00
            // BIS = 0
            // PCI HOME = 1
            // PCI ROAM = 1
            // BSPC = 0000
            // BSCA P = 000
            // END = 1
            return string_to_cvec("1 1 0 0 1 0 0 1 0 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 0 0");
        }
        void
        focc_impl::make_superframe() {
            superframe_frames.push_back(make_frame(overhead_word_1(), overhead_word_1()));
            superframe_frames.push_back(make_frame(overhead_word_2(), overhead_word_2()));
            superframe_frames.push_back(make_frame(access_type_parameters_global_action(), access_type_parameters_global_action()));
            superframe_frames.push_back(make_frame(control_filler_word(), control_filler_word(), false, true));
            superframe_frames.push_back(make_frame(control_filler_word(), control_filler_word(), false, true));
            superframe_frames.push_back(make_frame(control_filler_word(), control_filler_word(), false, true));
            superframe_frames.push_back(make_frame(control_filler_word(), control_filler_word(), false, true));
            superframe_frames.push_back(make_frame(control_filler_word(), control_filler_word(), false, true));
            superframe_frames.push_back(make_frame(control_filler_word(), control_filler_word(), false, true));
            superframe_frames.push_back(make_frame(control_filler_word(), control_filler_word(), false, true));
            superframe_frames.push_back(make_frame(control_filler_word(), control_filler_word(), false, true));
            superframe_frames.push_back(make_frame(control_filler_word(), control_filler_word(), false, true));
            superframe_frames.push_back(make_frame(control_filler_word(), control_filler_word(), false, true));
            superframe_frames.push_back(make_frame(control_filler_word(), control_filler_word(), false, true));
            superframe_frames.push_back(make_frame(control_filler_word(), control_filler_word(), false, true));
            superframe_frames.push_back(make_frame(control_filler_word(), control_filler_word(), false, true));
            superframe_frames.push_back(make_frame(control_filler_word(), control_filler_word(), false, true));
            superframe_frames.push_back(make_frame(control_filler_word(), control_filler_word(), false, true));
            superframe_frames.push_back(make_frame(control_filler_word(), control_filler_word(), false, true));

            cur_off = 0;
            cur_seg_idx = -1;   // XXX: hack because next_burst_state increments
            cur_seg_len = 0;
            cur_frame_idx = 0;
            cur_seg_data = NULL;
            cur_frame = superframe_frames[cur_frame_idx];
            if(cur_frame == NULL) {
                std::cout << "XXX cur_frame NULL" << std::endl;
                exit(1);
            }
            next_burst_state();
        }

        /* This method has been called when all the samples in the current 
         * burst segment have been sent.  It will update cur_burst_state, cur_off, and the 
         * relevant pointers within the current superframe.
         */
        void focc_impl::next_burst_state() {
            assert(cur_frame != NULL);
            cur_seg_idx = cur_seg_idx + 1;
            //std::cout << "XXX next_burst_state cur_seg_idx " << cur_seg_idx << " size " << cur_frame->segments.size() << std::endl;
            assert(cur_seg_idx <= cur_frame->segments.size());
            if(cur_seg_idx == cur_frame->segments.size()) {
                cur_frame_idx = cur_frame_idx + 1;
                assert(cur_frame_idx <= superframe_frames.size());      // XXX remove
                if(cur_frame_idx == superframe_frames.size()) {
                    cur_frame_idx = 0;
                }
                if(cur_frame->is_ephemeral) {
                    delete cur_frame;
                }
                cur_frame = superframe_frames[cur_frame_idx];
                if(cur_frame->is_filler) {
                    focc_frame *nframe = pop_frame_queue();
                    if(nframe != NULL) {
                        cur_frame = nframe;
                    }
                }
                cur_seg_idx = 0;
            }
            focc_segment *seg = cur_frame->segments[cur_seg_idx];
            cur_burst_state = seg->state;
            if(cur_burst_state == FOCC_MESSAGE) {
                cur_seg_len = seg->length;
                cur_seg_data = seg->encoded_data;
            } else {
                cur_seg_len = 0;
                cur_seg_data = NULL;
            }
            cur_off = 0;
        }

        void 
        focc_impl::focc_words_message(pmt::pmt_t msg) {
            assert(msg.is_tuple());
            size_t len = length(msg);
            assert(len > 2);
            long stream = to_long(tuple_ref(msg, 0));
            long nwords = to_long(tuple_ref(msg, 1));
            assert(nwords == len-2);
            for(long i = 0; i < nwords; i++) {
                pmt::pmt_t blob = tuple_ref(msg, 2+i);
                size_t blen = pmt::blob_length(blob);
                assert(blen == 28);
                const char *bdata = static_cast<const char *>(pmt::blob_data(blob));
                focc_frame *frame = NULL;
                switch(stream) {
                    case STREAM_A:
                        {
                            std::vector<char> word_a(bdata, bdata+blen);
                            std::vector<char> word_b = control_filler_word();
                            frame = make_frame(word_a, word_b, true, false);
                        }
                        break;
                    case STREAM_B:
                        {
                            std::vector<char> word_a = control_filler_word();
                            std::vector<char> word_b(bdata, bdata+blen);
                            frame = make_frame(word_a, word_b, true, false);
                        }
                        break;
                    case STREAM_BOTH:
                        {
                            std::vector<char> word_a(bdata, bdata+blen);
                            std::vector<char> word_b(bdata, bdata+blen);
                            frame = make_frame(word_a, word_b, true, false);
                        }
                        break;
                    default:
                        assert(0);
                        break;
                }
                push_frame_queue(frame);
            }
        }

        void 
        focc_impl::push_frame_queue(focc_frame *frame) {
            boost::mutex::scoped_lock lock(frame_queue_mutex);
            frame_queue.push(frame);
        }

        focc_frame *
        focc_impl::pop_frame_queue() {
            boost::mutex::scoped_lock lock(frame_queue_mutex);
            if(frame_queue.empty() == true) {
                return NULL;
            }
            focc_frame *frame = frame_queue.front();
            frame_queue.pop();
            return frame;
        }

        int
        focc_impl::work(int noutput_items,
                  gr_vector_const_void_star &input_items,
                  gr_vector_void_star &output_items) {
            unsigned char *out = (unsigned char *) output_items[0];
            // XXX: remove this
            const unsigned int samples_per_sym = d_symrate / 20000;

            if(noutput_items < 1) {
                std::cout << "noutput_items is empty: " << noutput_items << std::endl;
                return -1;
            }
            unsigned int optr = 0;
            int totalout = 0;

            int outleft = noutput_items;
            while(outleft > 0) {
                //printf("XXX YO optr %u c_b_s %d  cur_off %d\n", optr, cur_burst_state, cur_off);
                if(cur_burst_state == FOCC_BI_BIT) {
                    int samps_to_send = (samples_per_sym*2) - cur_off;
                    int toxfer = MIN(outleft, samps_to_send);
                    //printf("XXX s_to_s %d toxfer %d\n", samps_to_send, toxfer); 
                    assert(toxfer > 0);     // XXX: remove
                    assert(cur_off < (samples_per_sym*2));
                    static bool XXXdoneit = false;
                    if(busy_idle_bit == 0) {
                        //if(XXXdoneit == false) { XXXdoneit = true; printf("XXX YO DONE IT\n"); }
                        memcpy(&out[optr], &BI_zero_buf[cur_off], toxfer);
                    } else {
                        memcpy(&out[optr], &BI_one_buf[cur_off], toxfer);
                    }
                    optr += toxfer;
                    totalout += toxfer;
                    cur_off += toxfer;
                    outleft -= toxfer;
                    if(cur_off == (samples_per_sym*2)) {
                        next_burst_state();
                    }
                } else if(cur_burst_state == FOCC_MESSAGE) {
                    int samps_to_send = cur_seg_len - cur_off;
                    int toxfer = MIN(outleft, samps_to_send);
                    assert(toxfer > 0);     // XXX: remove
                    memcpy(&out[optr], &cur_seg_data[cur_off], toxfer);
                    optr += toxfer;
                    totalout += toxfer;
                    cur_off += toxfer;
                    outleft -= toxfer;
                    if(cur_off == cur_seg_len) {
                        next_burst_state();
                    }
                } else if(cur_burst_state == FOCC_END) {
                    next_burst_state();
                    return totalout;
                } else {
                    std::cout << "invalid value for cur_burst_state: " << cur_burst_state << std::endl;
                    assert(0);
                }
            }

#ifdef AMPS_DEBUG
            static unsigned int debugcount = 500000;
            if(totalout > 0 && debugcount > 0) {
                write(debugfd, out, totalout);
                debugcount -= totalout;
            }
#endif
            return totalout;
        }


        // Move data from our internal queue (d_bitqueue) out to gnuradio.  Here 
        // we also convert our data from bits (0 and 1) to symbols (1 and -1).  
        //
        // These symbols are then used by the FM block to generate signals that are
        // +/- the max deviation.  (For POCSAG, that deviation is 4500 Hz.)  All of
        // that is taken care of outside this block; we just emit -1 and 1.

        /*
        int
        focc_impl::oldwork(int noutput_items,
                  gr_vector_const_void_star &input_items,
                  gr_vector_void_star &output_items) {
            const float *in = (const float *) input_items[0];
            unsigned char *out = (unsigned char *) output_items[0];

            if(d_bitqueue.empty()) {
                return -1;
            }
            const int toxfer = noutput_items < d_bitqueue.size() ? noutput_items : d_bitqueue.size();
            assert(toxfer >= 0);
            for(int i = 0; i < toxfer ; i++) {
                const bool bbit = d_bitqueue.front();
                switch((int)bbit) {
                    case 0:
                        out[i] = -1;
                        break;
                    case 1:
                        out[i] = 1;
                        break;
                    default:
                        std::cout << "invalid value in bitqueue" << std::endl;
                        abort();
                        break;
                }
                d_bitqueue.pop();
            }
            return toxfer;

        }
        */
    } /* namespace amps */
} /* namespace gr */

