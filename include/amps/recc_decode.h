#ifndef INCLUDED_AMPS_RECC_DECODE_H
#define INCLUDED_AMPS_RECC_DECODE_H

#include <amps/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace amps {

    /*!
     * \brief <+description of block+>
     * \ingroup amps
     *
     */
    class AMPS_API recc_decode : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<recc_decode> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of amps::recc_decode.
       *
       * To avoid accidental use of raw pointers, amps::recc_decode's
       * constructor is in a private implementation
       * class. amps::recc_decode::make is the public interface for
       * creating new instances.
       */
      static sptr make();
    };

  } // namespace amps
} // namespace gr

#endif /* INCLUDED_AMPS_RECC_DECODE_H */

