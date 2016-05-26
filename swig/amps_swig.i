/* -*- c++ -*- */

#define AMPS_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "amps_swig_doc.i"

%{
#include "amps/focc.h"
#include "amps/recc.h"
%}


%include "amps/focc.h"
GR_SWIG_BLOCK_MAGIC2(amps, focc);
%include "amps/recc.h"
GR_SWIG_BLOCK_MAGIC2(amps, recc);
