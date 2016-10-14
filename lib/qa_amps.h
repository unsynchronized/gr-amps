#ifndef _QA_AMPS_H_
#define _QA_AMPS_H_

#include <gnuradio/attributes.h>
#include <cppunit/TestSuite.h>

//! collect all the tests for the gr-filter directory

class __GR_ATTR_EXPORT qa_amps
{
 public:
  //! return suite of tests for all of gr-filter directory
  static CppUnit::TestSuite *suite();
};

#endif /* _QA_AMPS_H_ */
