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

        /**
         * Decode a Manchester-encoded byte buffer (unsigned char buf of 0x0 
         * and 0x1 values) into another byte buffer.
         *
         * The length specified is the size of the destination buffer.
         * Returns the number of invalid bits.
         */
        size_t
        manchester_decode_binbuf(const unsigned char *srcbuf, unsigned char *dstbuf, size_t dstbufsz) {
            const size_t srcbufsz = dstbufsz * 2;
            size_t badbits = 0;
            assert(srcbufsz > dstbufsz);
            size_t o = 0;
            for(size_t i = 0; i < srcbufsz; i += 2) {
                const unsigned short sval = ((srcbuf[i] & 0xff) << 8) | (srcbuf[i+1] & 0xff);
                bool outbit;
                switch(sval) {
                    case 0x101:
                        outbit = 0;
                        badbits++;
                        break;
                    case 0x000:
                        outbit = 1;
                        badbits++;
                        break;
                    case 0x100:
                        outbit = 0;
                        break;
                    case 0x001:
                        outbit = 1;
                        break;
                    default:
                        assert(0);
                        break;
                }
                dstbuf[o] = outbit;
                o++;
            }
            return badbits;
        }

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
