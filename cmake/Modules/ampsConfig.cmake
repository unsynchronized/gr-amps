INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_AMPS amps)

FIND_PATH(
    AMPS_INCLUDE_DIRS
    NAMES amps/api.h
    HINTS $ENV{AMPS_DIR}/include
        ${PC_AMPS_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    AMPS_LIBRARIES
    NAMES gnuradio-amps
    HINTS $ENV{AMPS_DIR}/lib
        ${PC_AMPS_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(AMPS DEFAULT_MSG AMPS_LIBRARIES AMPS_INCLUDE_DIRS)
MARK_AS_ADVANCED(AMPS_LIBRARIES AMPS_INCLUDE_DIRS)

