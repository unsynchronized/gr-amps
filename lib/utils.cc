#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <itpp/comm/bch.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include "utils.h"

using namespace itpp;
using std::string;

namespace gr {
    namespace amps {
        std::vector<char>
        string_to_cvec(std::string binstr) {
            std::vector<char> outvec;
            for(string::const_iterator it = binstr.begin(); it != binstr.end(); ++it) {
                const char c = *it;
                switch(c) {
                    case '0':
                        outvec.push_back(0);
                        break;
                    case '1':
                        outvec.push_back(1);
                        break;
                    case ' ':
                        break;
                    case 0:
                        break;
                    default:
                        std::cerr << "invalid value: " << (int)c << std::endl;
                        throw std::invalid_argument("invalid character in bit vector string: expected 1, 0, or space");
                }
            }
            return outvec;
        }
        void charv_to_bvec(std::vector<char> &sv, bvec &bv) {
            assert(bv.size() >= sv.size());
            for(int i = 0; i < sv.size(); i++) {
                assert(sv[i] == 0 || sv[i] == 1);
                bv[i] = sv[i];
            }
        }
	}
}
