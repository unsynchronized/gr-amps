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
#include "command_processor_impl.h"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
#include "utils.h"

using namespace std;
using boost::shared_ptr;

namespace gr {
  namespace amps {

    command_processor::sptr
    command_processor::make()
    {
      return gnuradio::get_initial_sptr
        (new command_processor_impl());
    }

    /*
     * The private constructor
     */
    command_processor_impl::command_processor_impl()
      : bch(63, 2, true),
      gr::block("command_processor",
              gr::io_signature::make(0, 0, 0),
              gr::io_signature::make(0, 0, 0))
    {
        message_port_register_in(pmt::mp("commands"));
	  	set_msg_handler(pmt::mp("commands"),
			boost::bind(&command_processor_impl::commands_message, this, _1)
		);
        message_port_register_out(pmt::mp("focc_words"));
        message_port_register_out(pmt::mp("debug_output"));
    }

    void command_processor_impl::debug_msg(const char *msg) {
        pmt::pmt_t pdu = pmt::cons(pmt::make_dict(), pmt::init_u8vector(strlen(msg), (const uint8_t *)msg));
        message_port_pub(pmt::mp("debug_output"), pdu);

    }

    void command_processor_impl::handle_page(const std::string numstr) {
        std::cout << "XXX paging: *" << numstr << "*" << std::endl;
        debug_msg("--- paging!\n");
    }

    void command_processor_impl::commands_message(pmt::pmt_t msg) {
        pmt::pmt_t cmdvec = cdr(msg);
        if(is_u8vector(cmdvec) == false) {
            std::cout << "XXX command proc: got invalid message: " << msg << std::endl;
            return;
        }
        const std::vector<uint8_t> cmdchars = u8vector_elements(cmdvec);
        char carray[cmdchars.size()+1];
        memset(carray, 0, cmdchars.size()+1);
        for(size_t i = 0; i < cmdchars.size(); i++) {
            carray[i] = cmdchars[i];
        }
        std::string cmdstr(carray);
        if(boost::istarts_with(cmdstr, "page ")) {
            std::string num(cmdstr.substr(5));
            boost::trim(num);
            handle_page(num);
        } else {
            debug_msg("--- invalid command\n");
        }
    }

    /*
     * Our virtual destructor.
     */
    command_processor_impl::~command_processor_impl()
    {
    }

    void
    command_processor_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
      /* <+forecast+> e.g. ninput_items_required[0] = noutput_items */
    }

    int
    command_processor_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
        consume_each (noutput_items);

        return noutput_items;
    }

  } /* namespace amps */
} /* namespace gr */

