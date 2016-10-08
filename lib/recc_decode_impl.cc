/* -*- c++ -*- */
/* Written by Brandon Creighton <cstone@pobox.com>.
 *
 * This code is in the public domain; however, note that most of its
 * dependent code, including GNU Radio, is not.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "recc_decode_impl.h"
#include "utils.h"

using namespace std;
using boost::shared_ptr;

namespace gr {
  namespace amps {

    recc_decode::sptr
    recc_decode::make()
    {
      return gnuradio::get_initial_sptr
        (new recc_decode_impl());
    }

    /*
     * The private constructor
     */
    recc_decode_impl::recc_decode_impl()
      : bch(63, 2, true),
      gr::block("recc_decode",
              gr::io_signature::make(0, 0, 0),
              gr::io_signature::make(0, 0, 0))
    {
        message_port_register_in(pmt::mp("bursts"));
	  	set_msg_handler(pmt::mp("bursts"),
			boost::bind(&recc_decode_impl::bursts_message, this, _1)
		);
        message_port_register_out(pmt::mp("focc_words"));
    }

    /*
     * Convert a 48-bit value (one byte per bit) to a 36-bit value, after 
     * performing BCH decoding.
     */
    bool 
    recc_decode_impl::recc_bch_decode(const unsigned char *srcbuf, unsigned char *dstbuf) {
        bvec zeroes("0 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
        bvec srcbits(48);
        for(int i = 0; i < 48; i++) {
            const unsigned char b = srcbuf[i];
            assert(b == 0 || b == 1);
            srcbits[i] = b;
        }
        bvec padded(concat(zeroes, srcbits));
        cout << " padded: " << padded << endl;      // XXX
        bvec errbv;
        bvec decoded(51);
        bool retval = bch.decode(padded, decoded, errbv);
        cout << "decoded: " << decoded << endl;     // XXX
        bvec final = decoded(15, 50);
        cout << "  final: " << final << endl;       // XXX
        cout << "  errbv: " << errbv << endl;       // XXX
        for(int i = 0; i < 48; i++) {
            if(final[i] == 0) {
                dstbuf[i] = 0;
            } else {
                dstbuf[i] = 1;
            }
        }
        return retval;
    }

    void recc_decode_impl::bursts_message(pmt::pmt_t msg) {
        assert(pmt::is_blob(msg));
        size_t blen = pmt::blob_length(msg);
        const unsigned char *bdata = static_cast<const unsigned char *>(pmt::blob_data(msg));
        printf("XXX got blob len %zu  %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu\n", blen, 
                bdata[0], bdata[1], bdata[2], bdata[3], bdata[4], bdata[5], bdata[6],
                bdata[7], bdata[8], bdata[9], bdata[10], bdata[11], bdata[12], bdata[13]
                );
        unsigned char dcc[7];
        size_t dccerrbits = manchester_decode_binbuf(bdata, dcc, sizeof(dcc));
        // XXX: validate DCC
        unsigned char words[7][240];
        unsigned char decwords[7][48];
        size_t errs[7];
        bool validwords[7];
        for(int i = 0; i < 7; i++) {
            errs[i] = manchester_decode_binbuf(&bdata[14 + (480 * i)], words[i], 240);
            printf("%d: %zu errs\n", i, errs[i]);
        }
        for(int w = 0; w < 7; w++) {
            for(int r = 0; r < 5; r++) {
                validwords[w] = recc_bch_decode(&words[w][(r * 48)], decwords[w]);
                if(validwords[w] == true) {
                    break;
                }
            }
        }
        if(validwords[0] == false) {
            LOG_DEBUG("got a burst with an invalid Word A");
            return;
        }
        recc_word_a worda(words[0]);
        if(worda.E == false) {
            LOG_WARNING("got a RECC message with E=0; not sure what this is");
            return;
        }
        recc_word_b wordb(words[1]);
        // XXX: verify NAWC
        
        if(wordb.ORDER == 0 && wordb.ORDQ == 0 && wordb.MSG_TYPE == 0) {
            // This is a registration.  Word D will be sent.
            // XXX: verify NAWC
            unsigned char nawc = wordb.NAWC;
            unsigned long esn = 0;
            unsigned int nextword = 2;
            if(worda.S == true) {
                recc_word_c_serial wordc(words[nextword]);
                nextword++;
                esn = wordc.SERIAL;
                nawc = wordc.NAWC;
            }
            // XXX: verify NAWC
            if(nawc < 1 || nawc > 4) {
                LOG_WARNING("invalid NAWC value in RECC origination: 0x%x", nawc);
                return;
            }
            string dialed = "";
            for( ; nawc > 0; nawc--) {
                recc_word_called curword(words[nextword]);
                nextword++;
                dialed = dialed + curword.digits();
            }
            handle_origination(worda, wordb, esn, dialed);
        } else {
            LOG_WARNING("got unknown RECC message: ORDER 0x%hhx  ORDQ 0x%hhx  MSG_TYPE 0x%hhx", wordb.ORDER, wordb.ORDQ, wordb.MSG_TYPE);
        }
    }

    /**
     * Handle an Origination message.  
     *
     * As per 2.6.3.8, after a MS sends this, it'll be waiting for one of the
     * following messages on the control channel:
     *
     *     - Initial Voice Designation Message (a Mobile Station Control 
     *       Message (3.7.1.1) with a non-11 SCC and a VMAC/CHAN)
     *     - Directed Retry Message
     *     - Intercept
     *     - Reorder
     */
    void recc_decode_impl::handle_origination(recc_word_a &worda, recc_word_b &wordb, unsigned long esn, std::string dialed) {
        string reqmin = calc_min(worda, wordb);
        LOG_DEBUG("origination: MIN=%s ESN=%lx dialed %s", reqmin.c_str(), esn, dialed.c_str());
        long stream;
        char c = reqmin.back() - '0';
        if(c & 1) {
            stream = STREAM_B;
        } else {
            stream = STREAM_A;
        }

        unsigned char word1[28], word2[28];
        // Initial Voice Designation: Word 1 + Word 2 with SCC != 11
        const unsigned char vmac = 0;
        const unsigned short chan = 400;    // XXX: 400 is fwd 882.000 rev 837.000

        focc_word1(word1, true, GLOBAL_DCC_SHORT, worda.MIN1);
        focc_word2_voice_channel(word2, GLOBAL_SCC, wordb.MIN2, vmac, chan);

        pmt::pmt_t tuple = pmt::make_tuple(pmt::from_long(stream), pmt::from_long(2), pmt::mp(word1, 28), pmt::mp(word2, 28));
        message_port_pub(pmt::mp("focc_words"), tuple);
    }

    /**
     * Generate a 28-bit (1 byte/bit) array of the Mobile Station Control 
     * Message Word 1 (Abbreviated Address Word).  Output in word.
     *
     * If multiword is true, then this is the first word of many in a message;
     * if false, Word 1 is the only word in the message.
     */
    void focc_word1(unsigned char *word, const bool multiword, const unsigned char dcc, const u_int64_t MIN1) {
        word[0] = 0;
        word[1] = multiword ? 1 : 0;
        word[2] = ((dcc & 0x2) == 0x2) ? 1 : 0;
        word[3] = ((dcc & 0x1) == 0x1) ? 1 : 0;
        expandbits(&word[4], 24, MIN1);
    }

    /**
     * Generate a 28-bit (1 byte/bit) array of the Mobile Station Control 
     * Message Word 2 (VMAC/CHAN variant).
     */
    void focc_word2_voice_channel(unsigned char *word, const unsigned char scc, const u_int64_t MIN2, const unsigned char vmac, const unsigned short chan) {
        word[0] = 1;
        word[1] = 0;
        word[2] = ((scc & 0x2) == 0x2) ? 1 : 0;
        word[3] = ((scc & 0x1) == 0x1) ? 1 : 0;

        expandbits(&word[4], 10, MIN2);

        word[14] = ((vmac & 0x4) == 0x4) ? 1 : 0;
        word[15] = ((vmac & 0x2) == 0x2) ? 1 : 0;
        word[16] = ((vmac & 0x1) == 0x1) ? 1 : 0;

        expandbits(&word[17], 11, chan);
    }

    /*
     * Our virtual destructor.
     */
    recc_decode_impl::~recc_decode_impl()
    {
    }

    void
    recc_decode_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      /* <+forecast+> e.g. ninput_items_required[0] = noutput_items */
    }

    int
    recc_decode_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
        consume_each (noutput_items);

        return noutput_items;
    }

  } /* namespace amps */
} /* namespace gr */

