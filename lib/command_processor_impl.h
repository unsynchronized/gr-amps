/* -*- c++ -*- */
/* Written by Brandon Creighton <cstone@pobox.com>.
 *
 * This code is in the public domain; however, note that most of its
 * dependent code, including GNU Radio, is not.
 */

#ifndef INCLUDED_AMPS_COMMAND_PROCESSOR_IMPL_H
#define INCLUDED_AMPS_COMMAND_PROCESSOR_IMPL_H

#include <itpp/comm/bch.h>
#include <amps/command_processor.h>
#include "amps_packet.h"

using namespace itpp;

namespace gr {
  namespace amps {

    class command_processor_impl : public command_processor
    {
     private:
         itpp::BCH bch;

         void debug_msg(const char *msg);
        void handle_page(const std::string numstr);

     public:
      command_processor_impl();
      ~command_processor_impl();

      // Where all the action really happens
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);

      void commands_message(pmt::pmt_t msg);
    };

  } // namespace amps
} // namespace gr

#endif /* INCLUDED_AMPS_COMMAND_PROCESSOR_IMPL_H */

