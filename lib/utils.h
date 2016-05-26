#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/types.h>
#include <gnuradio/io_signature.h>
#include <itpp/comm/bch.h>
#include <iostream>
#include <sstream>
#include <vector>

using boost::shared_ptr;
using itpp::bvec;
using std::string;
using std::vector;
using std::ostringstream;

namespace gr {
    namespace amps {
        void charv_to_bvec(std::vector<char> &sv, bvec &bv);
        std::vector<char> string_to_cvec(std::string binstr);
    }
}

