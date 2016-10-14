########################################################################
# Include python install macros
########################################################################
include(GrPython)
if(NOT PYTHONINTERP_FOUND)
    return()
endif()

########################################################################
# Install python sources
########################################################################
GR_PYTHON_INSTALL(
    FILES
    __init__.py
    DESTINATION ${GR_PYTHON_DIR}/amps
)

########################################################################
# Handle the unit tests
########################################################################
include(GrTest)

set(GR_TEST_TARGET_DEPS gnuradio-amps)
set(GR_TEST_PYTHON_DIRS ${CMAKE_BINARY_DIR}/swig)
