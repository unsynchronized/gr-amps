#ifndef INCLUDED_AMPS_RECC_H
#define INCLUDED_AMPS_RECC_H

#include <amps/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace amps {

    /*!
     * \brief <+description of block+>
     * \ingroup amps
     *
     */
    class AMPS_API recc : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<recc> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of amps::recc.
       *
       * To avoid accidental use of raw pointers, amps::recc's
       * constructor is in a private implementation
       * class. amps::recc::make is the public interface for
       * creating new instances.
       */
      static sptr make();
    };

  } // namespace amps
} // namespace gr

#endif /* INCLUDED_AMPS_RECC_H */

