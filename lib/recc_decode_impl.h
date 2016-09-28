/* -*- c++ -*- */
/* Written by Brandon Creighton <cstone@pobox.com>.
 *
 * This code is in the public domain; however, note that most of its
 * dependent code, including GNU Radio, is not.
 */

#ifndef INCLUDED_AMPS_RECC_DECODE_IMPL_H
#define INCLUDED_AMPS_RECC_DECODE_IMPL_H

#include <amps/recc_decode.h>

namespace gr {
  namespace amps {

    class recc_decode_impl : public recc_decode
    {
     private:
      // Nothing to declare in this block.

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
    };

  } // namespace amps
} // namespace gr

#endif /* INCLUDED_AMPS_RECC_DECODE_IMPL_H */

