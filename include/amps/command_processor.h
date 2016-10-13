#ifndef INCLUDED_AMPS_COMMAND_PROCESSOR_H
#define INCLUDED_AMPS_COMMAND_PROCESSOR_H

#include <amps/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace amps {

    class AMPS_API command_processor : virtual public gr::block
    {
     public:
      typedef boost::shared_ptr<command_processor> sptr;

      static sptr make();
    };

  } // namespace amps
} // namespace gr

#endif /* INCLUDED_AMPS_COMMAND_PROCESSOR_H */

