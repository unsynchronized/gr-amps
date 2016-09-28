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
      : gr::block("recc_decode",
              gr::io_signature::make(0, 0, 0),
              gr::io_signature::make(0, 0, 0))
    {
        message_port_register_in(pmt::mp("bursts"));
	  	set_msg_handler(pmt::mp("bursts"),
			boost::bind(&recc_decode_impl::bursts_message, this, _1)
		);
    }

    void recc_decode_impl::bursts_message(pmt::pmt_t msg) {
        pmt::print(msg);
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

