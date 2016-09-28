/* Written by Brandon Creighton <cstone@pobox.com>.
 *
 * This code is in the public domain; however, note that most of its
 * dependent code, including GNU Radio, is not.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "recc_impl.h"
#include <string.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <vector>
#include "utils.h"

using std::ptrdiff_t;
using std::vector;
using std::string;
using std::cout;
using std::endl;
using boost::shared_ptr;

namespace gr {
    namespace amps {
        recc::sptr
        recc::make() {
            return gnuradio::get_initial_sptr (new recc_impl());
        }

        void printout(unsigned char *srcbuf, unsigned long len) {
            char outbuf[len+1];

            for(unsigned long i = 0; i < len; i++) {
                if(srcbuf[i] == 0) {
                    outbuf[i] = '0';
                } else if(srcbuf[i] == 1) {
                    outbuf[i] = '1';
                } else {
                    assert(0);
                }
            }
            outbuf[len] = 0;
            puts(outbuf);
        }

        void manchester_encode(const char *src, const size_t srclen, unsigned char *dst) {
            size_t o = 0;
            for(size_t i = 0; i < srclen; i++) { 
                if(src[i] == '0') {
                    dst[o] = 1;
                    dst[o+1] = 0;
                } else if(src[i] == '1') {
                    dst[o] = 0;
                    dst[o+1] = 1;
                } else {
                    assert(0);
                }
                o += 2;
            }
        }

        recc_impl::recc_impl()
          : d_symbufsz(65536), d_symbuflen(0),
          d_windowsz(4096), d_curstart(NULL), capture_len(1000),
          sync_block("recc",
                  io_signature::make(1, 1, sizeof (unsigned char)),
                  io_signature::make(0, 0, 0))
        {
            d_symbuf = new unsigned char[d_symbufsz]();
            const char *trigbuf = "1010101010101010101010101011100010010";
            trigger_len = strlen(trigbuf) * 2;
            trigger_data = new unsigned char[trigger_len]();
            manchester_encode(trigbuf, strlen(trigbuf), trigger_data);
            XXXbitcount = 0;

            message_port_register_out(pmt::mp("bursts"));
        }

        recc_impl::~recc_impl()
        {
            delete []d_symbuf;
            delete []trigger_data;
        }



        int
        recc_impl::work(int noutput_items,
                  gr_vector_const_void_star &input_items,
                  gr_vector_void_star &output_items) {
            const unsigned char *in = (const unsigned char *)input_items[0];

            if(noutput_items < 1) {
                printf("XXX noutput_items is %d\n", noutput_items);
                return 0;
            }
            assert(noutput_items < (d_symbufsz-d_windowsz));     // if this fails, just make the bufsz values bigger
            if((d_symbuflen + noutput_items) > d_symbufsz) {
                memmove(d_symbuf, &d_symbuf[d_symbufsz - d_windowsz], d_windowsz);
                d_symbuflen = d_windowsz;
                d_curstart = NULL;
            }
            assert((d_symbuflen + noutput_items) <= d_symbufsz);
            memmove(&d_symbuf[d_symbuflen], in, noutput_items);
            d_symbuflen += noutput_items;

            consume_each(noutput_items);
            if(d_symbuflen > trigger_len) {
                size_t searchsz = MIN(d_symbuflen, (noutput_items + trigger_len - 1));
                assert(searchsz <= d_symbufsz && searchsz <= d_symbuflen);
                if(d_curstart == NULL) {
                    d_curstart = (unsigned char *)memmem(&d_symbuf[d_symbuflen - searchsz], searchsz, trigger_data, trigger_len);
                }

                if(d_curstart != NULL) {
                    ptrdiff_t startoff = (d_curstart - d_symbuf);
                    busy_idle_bit = 0;
                    ptrdiff_t capturedsyms = d_symbuflen - startoff - trigger_len;
                    if(capturedsyms > capture_len) {
                        message_port_pub(pmt::mp("bursts"), pmt::mp("yoyo 123"));
                        printf("XXX YO GOT IT off %tu  d_symbuflen %zu  capturedsyms %tu  trigger_len %zu   bit @%lu  d_symbuf %p  noutput_items %d  trigger_len %lu  searchsz %lu\n", startoff, d_symbuflen, capturedsyms, trigger_len, XXXbitcount, d_symbuf, noutput_items, trigger_len, searchsz);
                        ptrdiff_t tomove = d_symbuflen - (capturedsyms + trigger_len);
                        assert(tomove >= 0);
                        if(tomove > 0) {
                            memmove(d_symbuf, &d_symbuf[(capturedsyms + trigger_len)], tomove);
                        }
                        d_symbuflen -= tomove;
                        d_curstart = NULL;
                        //printout(trigger_data, trigger_len);
                        //printout(start, trigger_len);
                    }
                }
            }

            XXXbitcount += noutput_items;

            return 0;
        }


    } /* namespace amps */
} /* namespace gr */

