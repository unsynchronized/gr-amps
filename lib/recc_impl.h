/* Written by Brandon Creighton <cstone@pobox.com>.
 *
 * This code is in the public domain; however, note that most of its
 * dependent code, including GNU Radio, is not.
 */

#ifndef INCLUDED_AMPS_RECC_IMPL_H
#define INCLUDED_AMPS_RECC_IMPL_H

#include <amps/recc.h>
#include <queue>
#include "amps_common.h"

using std::string;
using boost::shared_ptr;

#ifndef MAX
#define MAX(x,y) ((x)>(y)?(x):(y))
#endif /* MAX */

#ifndef MIN
#define MIN(x,y) ((x)<(y)?(x):(y))
#endif /* MIN */

namespace gr {
  namespace amps {
      
    class recc_impl : public recc
    {
    private:
        size_t d_symbufsz;          // size of d_symbuf
        unsigned char *d_symbuf;    // buffer of incoming symbols
        size_t d_symbuflen;         // current length of d_symbuf

        // The minimum number of previous symbols to keep around; practically,
        // the largest size you might want to extract at once minus one.
        size_t d_windowsz;

        size_t trigger_len;
        unsigned char *trigger_data;
        unsigned long XXXbitcount;

    public:
      recc_impl();
      ~recc_impl();

        int work(int noutput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
    };


  } // namespace amps
} // namespace gr

#endif /* INCLUDED_AMPS_RECC_IMPL_H */

