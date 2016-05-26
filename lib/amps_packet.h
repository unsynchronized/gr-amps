/* Written by Brandon Creighton <cstone@pobox.com>.
 *
 * This code is in the public domain; however, note that most of its
 * dependent code, including GNU Radio, is not.
 */

#ifndef AMPS_PACKET_H
#define AMPS_PACKET_H

/*
 * FOCC frames are broken down into bursts; each burst is f broken down 
 * into segments.  Segments are comprised of either busy-idle bits (FOCC_BI_BIT)
 * or message data (FOCC_MESSAGE; could be part of a word, dotting, or word 
 * sync).  After each burst is a FOCC_END.
 */
namespace gr {
  namespace amps {
      enum burst_state {
            FOCC_BI_BIT = 1,
            FOCC_MESSAGE = 2,
            FOCC_END = 3            // End of the current burst.
      };

      /**
       * focc_segment's data is manchester-encoded.
       */
      struct focc_segment {
          burst_state state;
          int length;
          char *encoded_data;

          focc_segment(burst_state nstate)
            : state(nstate), length(0), encoded_data(NULL) { }

          focc_segment(char *ndata, int nlength, int samples_per_sym) 
              : state(FOCC_MESSAGE) {
              length = nlength * 2 * samples_per_sym;
              encoded_data = new char[length];
              int optr = 0;
              for(int i = 0; i < nlength; i++) {
                  if(ndata[i] == 0) {
                      for(int j = 0; j < samples_per_sym; j++) {
                          encoded_data[optr] = 1;
                          optr++;
                      }
                      for(int j = 0; j < samples_per_sym; j++) {
                          encoded_data[optr] = -1;
                          optr++;
                      }
                  } else if(ndata[i] == 1) {
                      for(int j = 0; j < samples_per_sym; j++) {
                          encoded_data[optr] = -1;
                          optr++;
                      }
                      for(int j = 0; j < samples_per_sym; j++) {
                          encoded_data[optr] = 1;
                          optr++;
                      }
                  } else {
                      std::cout << "XXX invalid value: " << ndata[i] << std::endl;
                      assert(0);
                  }
              }
          }

          ~focc_segment() {
              if(encoded_data != NULL) {
                  delete []encoded_data;
              }
          }
      };
      class focc_frame {
          public:
          std::vector<focc_segment *> segments;
          focc_frame(std::vector<focc_segment *> nsegs) : segments(nsegs) { }
          ~focc_frame() {
              for(int i = 0; i < segments.size(); i++) {
                  if(segments[i] != NULL) {
                      delete segments[i];
                  }
                  segments[i] = NULL;
              }
          }
      };

  }
}


#endif /* AMPS_PACKET_H */
