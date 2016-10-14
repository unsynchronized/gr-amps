/*
 * This class gathers together all the test cases for the gr-filter
 * directory into a single test suite.  As you create new test cases,
 * add them here.
 */

#include "qa_amps.h"

CppUnit::TestSuite *
qa_amps::suite()
{
  CppUnit::TestSuite *s = new CppUnit::TestSuite("amps");

  return s;
}
