Commands
=====

In `rivet-fcgi` Tcl commands have below 3 groups:

* Implement in C (rivetCore.c and librivet),
  include in Rivet package
* Implement in a Tcl file, include in Rivet package
* Tcl packages


# Implement in C

In rivetCore.c:

* ::rivet::abort_code
* ::rivet::abort_page
* ::rivet::env
* ::rivet::headers
* ::rivet::include
* ::rivet::no_body
* ::rivet::parse
* ::rivet::raw_post
* ::rivet::var
* ::rivet::var_qs
* ::rivet::var_post
* ::rivet::upload

In librivet:

* ::rivet::escape_sgml_chars
* ::rivet::escape_shell_command
* ::rivet::escape_string
* ::rivet::unescape_string
* ::rivet::lassign_array
* ::rivet::lremove


# Implement in a Tcl file

* ::rivet::catch
* ::rivet::clock_to_rfc850_gmt (in cookie.tcl)
* ::rivet::cookie
* ::rivet::html
* ::rivet::http_accept
* ::rivet::import_keyvalue_pairs
* ::rivet::lempty
* ::rivet::lmatch
* ::rivet::load_cookies
* ::rivet::load_response
* ::rivet::parray
* ::rivet::random
* ::rivet::read_file
* ::rivet::redirect
* ::rivet::try
* ::rivet::url_query
* ::rivet::wrap
* ::rivet::wrapline (in wrap.tcl)
* ::rivet::xml

`::rivet::redirect` is an interesting command. This command use
`::rivet::headers`, `::rivet::no_body` and `::rivet::abort_page`
to implement its function.


# Tcl packages

* asciiglyphs
* calendar
* dio
* entities
* form
* formbroker
* session
* tclrivet

For DIO package:

DIO 1.1 supported databases -
* Mysql based on Tcl package Mysqltcl
* Oracle based on Tcl package Oratcl
* Postgresql based on Tcl package Pgtcl
* Sqlite based on Tcl package sqlite3

For DIO 1.2 I just add TDBC interface driver.

For session package:  
The session code by default requires a DIO handle called DIO.

Now rivet-fcgi does not implement ChildInitScript related code,
so you need to do below such things in your rvt file or
use `source` to load a tcl file for your related code

For example,

	<?
	  package require DIO
	  ::DIO::handle Tdbc Postgres DIO -user www -pass secret -db www

	  package require Session; Session SESSION -cookieLifetime 120 -debugMode 1
	?>

I also change session package config `debugFile` from stdout to stderr.

