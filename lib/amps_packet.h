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

      class recc_word {
          public:
          bool F;               // First - 1 when it's the first word in a message
          unsigned char NAWC;   // Number of Additional Words Coming

          recc_word(unsigned char *bytebuf) {
              F = ((bytebuf[0] & 1) == 1);
              NAWC = ((bytebuf[1] & 1) << 2)
                  | ((bytebuf[2] & 1) << 1)
                  | ((bytebuf[3] & 1) << 1);
          }
      };

#define BIT(x) ((x) & 0x1)

      inline unsigned char get8(const unsigned char *buf, size_t bits) {
          unsigned char val = 0;
          for(int i = 0; bits > 0; i++, bits--) {
              val <<= 1;
              val |= (buf[i] & 1);
          }
          return val;
      }

      inline u_int64_t get64(const unsigned char *buf, size_t bits) {
          u_int64_t val = 0;
          for(int i = 0; bits > 0; i++, bits--) {
              val <<= 1;
              val |= (buf[i] & 1);
          }
          return val;
      }

      class recc_word_a : public recc_word {
          public:
          bool T;             // when 1, message is an origination or an order; when 0, msg is a response
          bool S;             // when 1, the serial number is sent
          bool E;             // when 1, the extended address word (B) is sent
          bool ER;            // Extended Protocol Reverse Channel
          unsigned char SCM;  // station class mark (bits 3-0); section 2.3.3
          u_int64_t MIN1;     // first part of the MIN (bits 23-0);

          recc_word_a(unsigned char *bytebuf) : recc_word(bytebuf) {
              T = BIT(bytebuf[4]);
              S = BIT(bytebuf[5]);
              E = BIT(bytebuf[6]);
              ER = BIT(bytebuf[7]);
              SCM = get8(&bytebuf[8], 4);
              MIN1 = get64(&bytebuf[12], 24);
          }
      };

      class recc_word_b : public recc_word {
          public:
          unsigned char MSG_TYPE;   // also used and known as LOCAL; values in table 3.7.1-1
          unsigned char ORDQ;       // order qualifier field (table 3.7.1-1)
          unsigned char ORDER;      // order type (table 3.7.1-1)
          bool LT;                  // last-try field (2.6.3.8)
          bool EP;                  // Extended Protocol capable
          unsigned char SCM4;       // 4th bit of the SCM
          unsigned char MPCI;       // 00 AMPS only; 01 TIA/EIA 627 dual-mode; 10 TIA/EIA-95 dual mode; 11 IS-136 dual mode
          unsigned char SDCC1;      // must match the BS's SDCC1/2 values
          unsigned char SDCC2;      // must match the BS's SDCC1/2 values
          u_int64_t MIN2;           // second part of the MIN (bits 33-24)

          recc_word_b(unsigned char *bytebuf) : recc_word(bytebuf) {
              MSG_TYPE = get8(&bytebuf[4], 5);
              ORDQ = get8(&bytebuf[9], 3);
              ORDER = get8(&bytebuf[12], 5);
              LT = BIT(bytebuf[17]);
              EP = BIT(bytebuf[18]);
              SCM4 = bytebuf[19];
              MPCI = get8(&bytebuf[20], 2);
              SDCC1 = get8(&bytebuf[22], 2);
              SDCC2 = get8(&bytebuf[24], 2);
              MIN2 = get64(&bytebuf[26], 10);
          }
      };

      // Extract three MIN digits using the procedure in section 2.3.1.1-1.
      inline std::string extract_min_3(u_int64_t val) {
          std::string digs = "";
          u_int64_t m2 = val + 111;
          u_int64_t dig = (m2 % 10);
          digs = (char)(0x30 + dig) + digs;
          if(dig == 0) {
              m2 -= 10;
          } else {
              m2 -= dig;
          }

          dig = (m2 % 100) / 10;
          digs = (char)(0x30 + dig) + digs;
          if(dig == 0) {
              m2 -= 100;
          } else {
              m2 -= (m2 % 100);
          }

          dig = m2 / 100;
          if(dig > 9) {
              dig = 0;
          }
          digs = (char)(0x30 + dig) + digs;
          return digs;
      }

      /**
       * Given a RECC Word A (MIN1) and Word B (MIN2), calculate the MIN.
       */
      inline std::string calc_min(const recc_word_a &wa, const recc_word_b &wb) {
          std::string npa = extract_min_3(wb.MIN2);
          std::string exchange = extract_min_3((wa.MIN1 >> 14) & 0x3ff);
          std::string last_three = extract_min_3(wa.MIN1 & 0x3ff);
          u_int64_t thous = (wa.MIN1 >> 10) & 0xf;
          if(thous > 9) {
              thous = 0;
          }
          return npa + exchange + (char)(0x30 + thous) + last_three;
      }

  }
}


#endif /* AMPS_PACKET_H */
