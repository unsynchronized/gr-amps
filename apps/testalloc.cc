#include <iostream>
#include <vector>
#include <itpp/comm/bch.h>
#include <itpp/base/vec.h>
#include "lib/focc_impl.h"

using namespace std;
using namespace gr::amps;
using namespace itpp;


void blah() {
    bvec zeroes("0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0");
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
std::vector<char> overhead_word_1() {
    return string_to_cvec("1 1 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 1 0 1 0 0 0 1 1 1 0");
}


int main(int argc, char *argv[]) {
    for(int i = 0; i < 1000; i++) {
        std::vector<char> ow1 = overhead_word_1();
        std::vector<char> ow2 = overhead_word_1();
        blah();
    }

    unsigned long symrate = 200000;
    const unsigned int samples_per_sym = symrate / 20000;
    focc_impl *ai = new focc_impl(symrate);
    vector<const void *> ptr;
    vector<void *> outbuf(1);
    char sampbuf[10240], symbuf[10240];
    outbuf[0] = &sampbuf;

    int bitstoget = 20000;

    while(bitstoget > 0) {
        memset(symbuf, 0, sizeof(symbuf));
        memset(sampbuf, 0, sizeof(sampbuf));
        int retval = ai->work(sizeof(sampbuf), ptr, outbuf);
        assert((retval % samples_per_sym) == 0);
        assert((retval % 2) == 0);

        int nsyms = retval / samples_per_sym;
        for(int i = 0; i < nsyms; i++) {
            for(int j = 1; j < samples_per_sym; j++) {
                assert(sampbuf[(samples_per_sym*i)+j] == sampbuf[samples_per_sym*i]);
            }
            symbuf[i] = sampbuf[samples_per_sym*i];
            if(symbuf[i] == 0) {
                printf("invalid value detected at symbol position %d (sample position %u)\n", i, samples_per_sym*i);
                assert(symbuf[i] != 0);
            }
        }
        for(int i = 0; i < nsyms; i += 2) {
            char outbyte = 0;
            switch(symbuf[i]) {
                case -1:
                    assert(symbuf[i+1] == 1);
                    outbyte = 1;
                    break;
                case 1:
                    assert(symbuf[i+1] == -1);
                    outbyte = 0;
                    break;
                default:
                    printf("invalid value at symbuf[%d]: %d\n", i, (int)symbuf[i]);
                    assert(0);
                    break;
            }
            printf("%d", (int)outbyte);
        }
        bitstoget -= (retval/samples_per_sym/2);
    }
    printf("\n");
    return 1;
}
