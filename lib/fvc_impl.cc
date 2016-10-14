/* Written by Brandon Creighton <cstone@pobox.com>.
 *
 * This code is in the public domain; however, note that most of its
 * dependent code, including GNU Radio, is not.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "fvc_impl.h"
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

        /**
         * AMPS BS FVC. 553 3.7.2.  
         */
        fvc::sptr
        fvc::make(unsigned long symrate) {
            return gnuradio::get_initial_sptr (new fvc_impl(symrate));
        }



        void 
        fvc_impl::queue(bvec &bv) {
            for(unsigned int i = 0; i < bv.size(); i++) {
                queuebit(bv[i]);
            }
        }
        void 
        fvc_impl::queue(gr_uint32 val) {
            for(int i = 0; i < 32; i++) {
                queuebit(((val & 0x80000000) == 0x80000000) ? 1 : 0);
                val = val << 1;
            }
        }



        fvc_impl::fvc_impl(unsigned long symrate)
          : timerhack(0), d_symrate(symrate), bch(63, 2, true), samples_per_sym(symrate / 20000),
          sync_block("fvc",
                  io_signature::make(0, 0, 0),
                  io_signature::make(1, 1, sizeof (unsigned char)))
        {
            message_port_register_in(pmt::mp("fvc_words"));
            set_msg_handler(pmt::mp("fvc_words"),
                boost::bind(&fvc_impl::fvc_words_message, this, _1)
            );
            message_port_register_out(pmt::mp("command_out"));
        }

        // Insert bits into the queue.  Here is also where we repeat a single bit
        // so that we're emitting d_symrate symbols per second.
        inline void 
        fvc_impl::queuebit(bool bit) {
            if(bit == 1) {
                for(unsigned int i = 0; i < samples_per_sym; i++) {
                    d_curdata.push(0);
                }
                for(unsigned int i = 0; i < samples_per_sym; i++) {
                    d_curdata.push(1);
                }
            } else {
                for(unsigned int i = 0; i < samples_per_sym; i++) {
                    d_curdata.push(1);
                }
                for(unsigned int i = 0; i < samples_per_sym; i++) {
                    d_curdata.push(0);
                }
            }
        }

        fvc_impl::~fvc_impl()
        {
        }

        /*
         * Convert a 28-bit vector (represented as one bit per char, values 
         * being numeric 0 and 1) to a 40-bit bch-encoded word.
         */
        void fvc_impl::fvc_bch(std::vector<char> inbits, bvec &outbv) {
            std::vector<char> outvec(40);
            bvec zeroes("0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
            bvec srcbvec(28);
            charv_to_bvec(inbits, srcbvec);
            bvec padded(concat(zeroes, srcbvec));
            bvec encoded = bch.encode(padded);
            outbv = encoded(23, encoded.size()-1);
            assert(outbv.size() == 40);
        }

        void fvc_impl::fvc_words_message(pmt::pmt_t msg) {
            printf("XXX: got new FVC words\n");
            assert(msg.is_tuple());
            size_t len = length(msg);
            assert(len > 1);
            long nwords = to_long(tuple_ref(msg, 0));
            vector<vector<char> > words;
            for(long i = 0; i < nwords; i++) {
                pmt::pmt_t blob = tuple_ref(msg, 1+i);
                size_t blen = pmt::blob_length(blob);
                assert(blen == 28);
                const char *bdata = static_cast<const char *>(pmt::blob_data(blob));
                vector<char> word(bdata, bdata+blen);
                words.push_back(word);
            }
            if(len > (1+nwords)) {
                timerhack = to_uint64(tuple_ref(msg, 1+nwords));
                printf("XXX: setting timerhack to %lld\n", timerhack);
            }
            bvec bigdot("1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1");
            bvec smalldot("1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1");
            bvec wsync("1 1 1 0 0 0 1 0 0 1 0");
            for(long i = 0; i < nwords; i++) {
                queue(bigdot);
                bvec encoded(40);
                fvc_bch(words[i], encoded);
                for(int j = 0; j < 11; j++) {
                    queue(wsync);
                    queue(encoded);
                    if(j < 10) {
                        queue(smalldot);
                    }
                }
            }
        }

        // Move data from our internal queue (d_curqueue) out to gnuradio.  Here 
        // we also convert our data from bits (0 and 1) to symbols (1 and -1).  
        //
        // These symbols are then used by the FM block to generate signals that are
        // +/- the max deviation.  (For POCSAG, that deviation is 4500 Hz.)  All of
        // that is taken care of outside this block; we just emit -1 and 1.

        int
        fvc_impl::work(int noutput_items,
                  gr_vector_const_void_star &input_items,
                  gr_vector_void_star &output_items) {
            const float *in = (const float *) input_items[0];
            unsigned char *out = (unsigned char *) output_items[0];

            if(d_curdata.empty()) {
                return noutput_items;
            }
            if(d_curqueue.empty()) {
                if(timerhack >= 1) {
                    timerhack--;
                    if(timerhack == 0) {
                        printf("XXX FVC LIMIT HIT\n");
                        const char *msg = "fvc off";
                        pmt::pmt_t pdu = pmt::cons(pmt::make_dict(), pmt::init_u8vector(strlen(msg), (const uint8_t *)msg));
                        message_port_pub(pmt::mp("command_out"), pdu);
                    }
                }
                d_curqueue = (const std::queue<bool> &)d_curdata;
            }
            const int toxfer = noutput_items < d_curqueue.size() ? noutput_items : d_curqueue.size();
            assert(toxfer >= 0);
            for(int i = 0; i < toxfer ; i++) {
                const bool bbit = d_curqueue.front();
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
                d_curqueue.pop();
            }
            return toxfer;
        }
    } /* namespace amps */
} /* namespace gr */

