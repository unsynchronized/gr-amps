/* Written by Brandon Creighton <cstone@pobox.com>.
 *
 * This code is in the public domain; however, note that most of its
 * dependent code, including GNU Radio, is not.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "amps_packet.h"
#include "utils.h"

using namespace std;
using boost::shared_ptr;

namespace gr {
  namespace amps {    
   /**
     * Generate a 28-bit (1 byte/bit) array of the Mobile Station Control 
     * Message Word 1 (Abbreviated Address Word).  Output in word.
     *
     * If multiword is true, then this is the first word of many in a message;
     * if false, Word 1 is the only word in the message.
     */
    void focc_word1(unsigned char *word, const bool multiword, const unsigned char dcc, const u_int64_t MIN1) {
        word[0] = 0;
        word[1] = multiword ? 1 : 0;
        word[2] = ((dcc & 0x2) == 0x2) ? 1 : 0;
        word[3] = ((dcc & 0x1) == 0x1) ? 1 : 0;
        expandbits(&word[4], 24, MIN1);
    }

    /**
     * Generate a 28-bit (1 byte/bit) array of the Mobile Station Control
     * Message Word 2 (SCC = 11)
     */
    void focc_word2_general(unsigned char *word, const u_int64_t MIN2, const unsigned char msg_type, const unsigned char ordq, const unsigned char order) {
        word[0] = 1;
        word[1] = 0;
        word[2] = 1;
        word[3] = 1;

        expandbits(&word[4], 10, MIN2);
        word[14] = 0;
        expandbits(&word[15], 5, msg_type);
        expandbits(&word[20], 3, ordq);
        expandbits(&word[23], 5, order);
    }

    /**
     * Generate a 28-bit (1 byte/bit) array of the Mobile Station Control 
     * Message Word 2 (VMAC/CHAN variant).
     */
    void focc_word2_voice_channel(unsigned char *word, const unsigned char scc, const u_int64_t MIN2, const unsigned char vmac, const unsigned short chan) {
        word[0] = 1;
        word[1] = 0;
        word[2] = ((scc & 0x2) == 0x2) ? 1 : 0;
        word[3] = ((scc & 0x1) == 0x1) ? 1 : 0;

        expandbits(&word[4], 10, MIN2);

        word[14] = ((vmac & 0x4) == 0x4) ? 1 : 0;
        word[15] = ((vmac & 0x2) == 0x2) ? 1 : 0;
        word[16] = ((vmac & 0x1) == 0x1) ? 1 : 0;

        expandbits(&word[17], 11, chan);
    }


  }
}
