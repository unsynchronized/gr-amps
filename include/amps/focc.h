/* Written by Brandon Creighton <cstone@pobox.com>.
 *
 * This code is in the public domain; however, note that most of its
 * dependent code, including GNU Radio, is not.
 */

#ifndef INCLUDED_AMPS_FOCC_H
#define INCLUDED_AMPS_FOCC_H

#include <amps/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace amps {

    /*!
     * \brief <+description of block+>
     * \ingroup amps
     *
     */
    class AMPS_API focc : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<focc> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of amps::focc.
       *
       * To avoid accidental use of raw pointers, amps::focc's
       * constructor is in a private implementation
       * class. amps::focc::make is the public interface for
       * creating new instances.
       */
      static sptr make(unsigned long symrate, bool aggressive_registration);
    };

  } // namespace amps
} // namespace gr

#endif /* INCLUDED_AMPS_FOCC_H */

