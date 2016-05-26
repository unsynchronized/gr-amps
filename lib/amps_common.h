#ifndef AMPS_COMMON_H
#define AMPS_COMMON_H

namespace gr {
  namespace amps {
    
    extern volatile bool busy_idle_bit;        // Figure 3.7.1-1: 1 when idle, 0 when busy

  }
}


#endif  /* AMPS_COMMON_H */
