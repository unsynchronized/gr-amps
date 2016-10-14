#ifndef INCLUDED_AMPS_API_H
#define INCLUDED_AMPS_API_H

#include <gnuradio/attributes.h>

#ifdef gnuradio_amps_EXPORTS
#  define AMPS_API __GR_ATTR_EXPORT
#else
#  define AMPS_API __GR_ATTR_IMPORT
#endif

#endif /* INCLUDED_AMPS_API_H */
