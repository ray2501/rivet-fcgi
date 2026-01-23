rivet-fcgi
=====

# Overview

[FastCGI](https://fastcgi-archives.github.io/) is a binary protocol for
interfacing interactive programs with a web server.
It is a language-independent protocol based on CGI.
If you want to write FastCGI application by using [Tcl](https://www.tcl.tk/),
there are several FastCGI for Tcl packages.
You can check [FastCGI](https://wiki.tcl-lang.org/page/FastCGI).

This is a research and prototype project.
The idea is to embed a Tcl interpreter and
[Rivet](https://tcl.apache.org/rivet/) tempalte parser in
a FastCGI application.

The bulk of this code is from a part of the
[Rivet](https://tcl.apache.org/rivet/) project.

So it looks like:

    Web server <---> FastCGI process manager <---> rivet-fcgi

I use [spawn-fcgi](https://github.com/lighttpd/spawn-fcgi)
when I test this project.
However, I think it should be OK to use other process manager.


# Build

You need to install CMake, Tcl development files (tcl-devel) and
FastCGI Developpement Kit (FastCGI-devel).
This project uses CMake to build source code.

Below is an example:

    mkdir build
    cd build
    cmake ..
    cmake --build .

Users can use `CMAKE_INSTALL_PREFIX`, `INSTALL_LIB_DIR`,
`INSTALL_BIN_DIR` and `RIVETLIB_DESTDIR` to specify installation directory.

For example:

    cmake -DCMAKE_INSTALL_PREFIX=/usr -DINSTALL_LIB_DIR=lib64 \
    -DRIVETLIB_DESTDIR=lib64/tcl/rivetfcgi0.3.0 ..


# Documentation

I use Rivet's [manuals](https://tcl.apache.org/rivet/html/manuals.html)
as my reference.


# Notice

If users do not use `::rivet::headers` command to specify Content-type,
the default header for .rvt file is:

    Content-type: text/html; charset=utf-8

And if file extension is .tcl file, users need to send http headers
by yourself.

Please notice some environment variables maybe do not exist (then result
in errors in your application). For example, it is possible that `LANG`
does not exist!


