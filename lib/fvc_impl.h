/* Written by Brandon Creighton <cstone@pobox.com>.
 *
 * This code is in the public domain; however, note that most of its
 * dependent code, including GNU Radio, is not.
 */

#ifndef INCLUDED_AMPS_FVC_IMPL_H
#define INCLUDED_AMPS_FVC_IMPL_H

#include <amps/fvc.h>
#include <queue>
#include <itpp/comm/bch.h>
#include "amps_packet.h"
#include "amps_common.h"

using namespace itpp;
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
      
    class fvc_impl : public fvc
    {
    private:
        std::queue<bool> d_curqueue;    // Queue of symbols to be sent out.
        unsigned long d_symrate;        // output symbol rate (must be evenly divisible by the baud rate)
        std::queue<bool> d_curdata;     // current words
        itpp::BCH bch;

        uint64_t timerhack;             // XXX HACK: When >0, after this many message blocks have been sent, sent "fvc off" to command_out

        const unsigned int samples_per_sym;

        inline void queuebit(bool bit);
        void fvc_bch(std::vector<char> inbits, bvec &outbv);

    public:
        fvc_impl(unsigned long symrate);
        ~fvc_impl();

        void fvc_words_message(pmt::pmt_t msg);
        void queue(bvec &bv);
        void queue(uint32_t val);
        int work(int noutput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
    };


  } // namespace amps
} // namespace gr

#endif /* INCLUDED_AMPS_FVC_IMPL_H */

