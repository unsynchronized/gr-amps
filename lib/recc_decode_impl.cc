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
        printf("XXX DCC: %hhu%hhu%hhu%hhu%hhu%hhu%hhu\n", 
                dcc[0], dcc[1], dcc[2], dcc[3], dcc[4], dcc[5], dcc[6]);
        // XXX: validate DCC
        unsigned char words[7][240];
        unsigned char decwords[7][48];
        size_t errs[7];
        for(int i = 0; i < 7; i++) {
            errs[i] = manchester_decode_binbuf(&bdata[14 + (480 * i)], words[i], 240);
            printf("%d: %zu errs\n", i, errs[i]);
        }
        for(int w = 0; w < 7; w++) {
            printf("--- word %d\n", w);
            for(int r = 0; r < 5; r++) {
                printf("%hhu %hhu %hhu %hhu %hhu %hhu %hhu %hhu  ", 
                        words[w][(r * 48) + 0],
                        words[w][(r * 48) + 1],
                        words[w][(r * 48) + 2],
                        words[w][(r * 48) + 3],
                        words[w][(r * 48) + 4],
                        words[w][(r * 48) + 5],
                        words[w][(r * 48) + 6],
                        words[w][(r * 48) + 7]
                );
                bool retval = recc_bch_decode(&words[w][(r * 48)], decwords[w]);
                if(retval == false) {
                    cout << "FAILED" << endl;
                }
            }
        }
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

