
########################################################################
# Check if there is C++ code at all
########################################################################
if(NOT amps_sources)
	MESSAGE(STATUS "No C++ sources... skipping swig/")
	return()
endif(NOT amps_sources)

########################################################################
# Include swig generation macros
########################################################################
find_package(SWIG)
find_package(PythonLibs 2)
if(NOT SWIG_FOUND OR NOT PYTHONLIBS_FOUND)
    return()
endif()
include(GrSwig)
include(GrPython)

########################################################################
# Setup swig generation
########################################################################
foreach(incdir ${GNURADIO_RUNTIME_INCLUDE_DIRS})
    list(APPEND GR_SWIG_INCLUDE_DIRS ${incdir}/gnuradio/swig)
endforeach(incdir)

set(GR_SWIG_LIBRARIES gnuradio-amps)
set(GR_SWIG_DOC_FILE ${CMAKE_CURRENT_BINARY_DIR}/amps_swig_doc.i)
set(GR_SWIG_DOC_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../include)

GR_SWIG_MAKE(amps_swig amps_swig.i)

########################################################################
# Install the build swig module
########################################################################
GR_SWIG_INSTALL(TARGETS amps_swig DESTINATION ${GR_PYTHON_DIR}/amps)

########################################################################
# Install swig .i files for development
########################################################################
install(
    FILES
    amps_swig.i
    ${CMAKE_CURRENT_BINARY_DIR}/amps_swig_doc.i
    DESTINATION ${GR_INCLUDE_DIR}/amps/swig
)
