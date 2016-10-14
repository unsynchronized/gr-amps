########################################################################
# Setup library
########################################################################
include(GrPlatform) #define LIB_SUFFIX

include_directories(
    ${Boost_INCLUDE_DIR}
    ${ITPP_INCLUDE_DIR}
)
link_directories(${Boost_LIBRARY_DIRS})

list(APPEND amps_sources
    focc_impl.cc
    fvc_impl.cc
    utils.cc
    recc_impl.cc
    amps_packet.cc
    command_processor_impl.cc
    recc_decode_impl.cc
)

set(amps_sources "${amps_sources}" PARENT_SCOPE)
if(NOT amps_sources)
	MESSAGE(STATUS "No C++ sources... skipping lib/")
	return()
endif(NOT amps_sources)

add_library(gnuradio-amps SHARED ${amps_sources})
target_link_libraries(gnuradio-amps ${Boost_LIBRARIES} ${GNURADIO_ALL_LIBRARIES} ${ITPP_LIBRARIES})
set_target_properties(gnuradio-amps PROPERTIES DEFINE_SYMBOL "gnuradio_amps_EXPORTS")

if(APPLE)
    set_target_properties(gnuradio-amps PROPERTIES
        INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib"
    )
endif(APPLE)

########################################################################
# Install built library files
########################################################################
install(TARGETS gnuradio-amps
    LIBRARY DESTINATION lib${LIB_SUFFIX} # .so/.dylib file
    ARCHIVE DESTINATION lib${LIB_SUFFIX} # .lib file
    RUNTIME DESTINATION bin              # .dll file
)

########################################################################
# Build and register unit test
########################################################################
include(GrTest)

include_directories(${CPPUNIT_INCLUDE_DIRS})

list(APPEND test_amps_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/test_amps.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/qa_amps.cc
)

add_executable(test-amps ${test_amps_sources})

target_link_libraries(
  test-amps
  ${GNURADIO_RUNTIME_LIBRARIES}
  ${Boost_LIBRARIES}
  ${CPPUNIT_LIBRARIES}
  ${ITPP_LIBRARIES}
  gnuradio-amps
)

GR_ADD_TEST(test_amps test-amps)
