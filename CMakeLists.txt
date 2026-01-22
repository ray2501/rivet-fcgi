cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

set(VERSION "0.2.3")
set(RIVETLIB_PACKAGE "rivetlib")
set(RIVETLIB_PACKAGE_VERSION "3.2")
set(INIT_VERSION "3.2.8")
set(RIVET_INIT_VERSION ${INIT_VERSION})

project (rivet-fcgi VERSION ${VERSION} LANGUAGES C)

option(TCL_THREADS "build with threads" ON)
option(BUILD_SHARED_LIBS "build and link with shared libraries" ON)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set (CMAKE_INSTALL_PREFIX /usr)
endif()

set(CPACK_SET_DESTDIR ON)
set(INSTALL_LIB_DIR lib64 CACHE PATH "Installation directory for libraries")
set(INSTALL_BIN_DIR bin CACHE PATH "Installation directory for executables")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(NOT DEFINED RIVETLIB_DESTDIR)
  set(RIVETLIB_DESTDIR "${INSTALL_LIB_DIR}/rivetfcgi${VERSION}")
endif(NOT DEFINED RIVETLIB_DESTDIR)

find_package(ECM 1.0.0 REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
include(GNUInstallDirs)
include(ECMUninstallTarget)
include(CPack)

find_package (TclStub 8.6.1 REQUIRED)
find_package(TCL 8.6.1 REQUIRED)
find_package(FCGI REQUIRED)

MESSAGE ( STATUS "  TCL_TCLSH:               " ${TCL_TCLSH} )
MESSAGE ( STATUS "  TCL_INCLUDE_PATH:        " ${TCL_INCLUDE_PATH} )
MESSAGE ( STATUS "  TCL_LIBRARY:             " ${TCL_LIBRARY} )
MESSAGE ( STATUS "  TCL_STUB_LIBRARY:        " ${TCL_STUB_LIBRARY} )
MESSAGE ( STATUS "  FCGI_LIBRARY:            " ${FCGI_LIBRARY} )
MESSAGE ( STATUS "  FCGI_INCLUDE_DIR:        " ${FCGI_INCLUDE_DIR} )

set(SRC_DIR "${PROJECT_SOURCE_DIR}/src")
set(RIVET_LIB_DIR "${PROJECT_SOURCE_DIR}/rivet")

set(INCLUDE_DIRS_PRIVATE
  "${SRC_DIR}"
  "${SRC_DIR}/parser"
  "${PROJECT_BINARY_DIR}"
)

add_subdirectory(src)

enable_testing()
add_test(
    NAME rivet_test
    COMMAND ${TCL_TCLSH} ${PROJECT_SOURCE_DIR}/tests/all.tcl
)
