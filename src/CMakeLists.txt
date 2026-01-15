# list out the sources for your project
set (rivetparser_sources parser/rivetParser.c parser/parserPkgInit.c)
set (rivetlib_sources librivet/rivetList.c librivet/rivetCrypt.c
     librivet/rivetWWW.c librivet/rivetPkgInit.c)
set (SRCS rivet-fcgi.c channel.c errChan.c helputils.c dataParser.c
     parser/rivetParser.c rivetCore.c tclWeb.c)


#  Generate config.h
# ===========================================================================
MESSAGE(STATUS "Generating: " ${SRC_DIR}/config.h)
configure_file("${SRC_DIR}/config.h.in"
               "${PROJECT_BINARY_DIR}/config.h" @ONLY)

#  Generate init.tcl
# ===========================================================================
MESSAGE(STATUS "Generating: " ${RIVET_GENERATE_DIR}/init.tcl)
configure_file("${RIVET_LIB_DIR}/init.tcl.in"
               "${PROJECT_BINARY_DIR}/init.tcl" @ONLY)

add_executable(rivet-fcgi ${SRCS})
add_library(rivetparser  ${rivetparser_sources})
add_library(rivetlib  ${rivetlib_sources})

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(rivet-fcgi PRIVATE
        -Wunused-variable
    )
endif()

if(TCL_THREADS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DTCL_THREADS")
endif(TCL_THREADS)

target_compile_definitions(rivetparser PUBLIC -DUSE_TCL_STUBS)
target_compile_definitions(rivetlib PUBLIC -DUSE_TCL_STUBS)
target_link_libraries (rivet-fcgi ${FCGI_LIBRARY} ${TCL_LIBRARY})
target_link_libraries (rivetparser ${TCL_STUB_LIBRARY})
target_link_libraries (rivetlib ${TCL_STUB_LIBRARY})
target_include_directories (rivet-fcgi
    PUBLIC ${FCGI_INCLUDE_DIR} ${TCL_INCLUDE_PATH}
    PRIVATE ${INCLUDE_DIRS_PRIVATE})
target_include_directories (rivetparser PUBLIC ${TCL_INCLUDE_PATH} PRIVATE ${INCLUDE_DIRS_PRIVATE} )
target_include_directories (rivetlib PUBLIC ${TCL_INCLUDE_PATH} PRIVATE ${INCLUDE_DIRS_PRIVATE} )

install(TARGETS rivetparser rivetlib DESTINATION ${RIVETLIB_DESTDIR})
install(TARGETS rivet-fcgi DESTINATION bin)

#  Install library directories...
# ===========================================================================
install(DIRECTORY   ${RIVET_LIB_DIR}/rivet-tcl
        DESTINATION ${RIVETLIB_DESTDIR})
install(DIRECTORY
        ${RIVET_LIB_DIR}/packages/asciiglyphs
        ${RIVET_LIB_DIR}/packages/calendar
        ${RIVET_LIB_DIR}/packages/dio
        ${RIVET_LIB_DIR}/packages/entities
        ${RIVET_LIB_DIR}/packages/form
        ${RIVET_LIB_DIR}/packages/formbroker
        ${RIVET_LIB_DIR}/packages/session
        ${RIVET_LIB_DIR}/packages/tclrivet
        DESTINATION ${RIVETLIB_DESTDIR}/packages)

install(FILES
        ${PROJECT_BINARY_DIR}/init.tcl
        DESTINATION ${RIVETLIB_DESTDIR})

file(WRITE ${PROJECT_BINARY_DIR}/regenerate_pkgIndex.tcl
"puts \"Regenerating pkgIndex.tcl in [pwd]:\"
file delete {*}[glob -nocomplain [file join packages * pkgIndex.tcl]] pkgIndex.tcl
pkg_mkIndex -verbose [pwd] init.tcl packages/*/*.tcl *[info sharedlibextension]"
)
install(CODE "
  execute_process(COMMAND ${CMAKE_COMMAND} -E copy \
      ${PROJECT_BINARY_DIR}/regenerate_pkgIndex.tcl \
      ${DESTDIR}${CMAKE_INSTALL_PREFIX}/${RIVETLIB_DESTDIR}/regenerate_pkgIndex.tcl)
  "
)
install(CODE "
  execute_process(COMMAND ${TCL_TCLSH} regenerate_pkgIndex.tcl
                  WORKING_DIRECTORY ${DESTDIR}${CMAKE_INSTALL_PREFIX}/${RIVETLIB_DESTDIR})
  file(REMOVE ${DESTDIR}${CMAKE_INSTALL_PREFIX}/${RIVETLIB_DESTDIR}/regenerate_pkgIndex.tcl)
  "
)
