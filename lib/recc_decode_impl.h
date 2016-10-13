/* -*- c++ -*- */
/* Written by Brandon Creighton <cstone@pobox.com>.
 *
 * This code is in the public domain; however, note that most of its
 * dependent code, including GNU Radio, is not.
 */

#ifndef INCLUDED_AMPS_RECC_DECODE_IMPL_H
#define INCLUDED_AMPS_RECC_DECODE_IMPL_H

#include <itpp/comm/bch.h>
#include <amps/recc_decode.h>
#include "amps_packet.h"

using namespace itpp;

namespace gr {
  namespace amps {

    class recc_decode_impl : public recc_decode
    {
     private:
         itpp::BCH bch;

         bool recc_bch_decode(const unsigned char *srcbuf, unsigned char *dstbuf);

     public:
      recc_decode_impl();
      ~recc_decode_impl();

      // Where all the action really happens
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);

      void bursts_message(pmt::pmt_t msg);
      void handle_origination(recc_word_a &worda, recc_word_b &wordb, unsigned long esn, std::string dialed);
      void handle_response(const recc_word_a &worda, const recc_word_b &wordb);
    };

  } // namespace amps
} // namespace gr

#endif /* INCLUDED_AMPS_RECC_DECODE_IMPL_H */

