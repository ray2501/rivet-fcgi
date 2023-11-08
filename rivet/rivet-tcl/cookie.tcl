##
## cookie [set|get] and supporting functions
##
## $Id$
##


namespace eval ::rivet {

## clock_to_rfc850_gmt seconds -- Convert an integer-seconds-since-1970
## click value to RFC850 format, with the additional requirement that it
## be GMT only.
##
    proc clock_to_rfc850_gmt {seconds} {
        return [clock format $seconds -format "%a, %d-%b-%y %T GMT" -gmt 1]
    }

## make_cookie_attributes paramsArray -- Build up cookie parameters.
##
## If an expires element appears in the array, it's appended to the
## result as "; expires=value"
##
## If an element is present named "days", "hours", or "minutes", the
## corresponding value is converted to seconds, added to the current
## time, converted to RFC850 time format, and appended to the result
## as, for example, "; expires=Sun, 25-Sep-05 23:09:52 GMT"
##
## If any other elements named "path", "domain", or "secure" are present,
## they too are appended to the result.
##
## The resut is returned.
##

    proc make_cookie_attributes {paramsArray} {
        upvar 1 $paramsArray params

        set cookieParams ""
        set expiresIn    0

        if { [info exists params(expires)] } {
            append cookieParams "; expires=$params(expires)"
        } else {
            foreach {time num} [list days 86400 hours 3600 minutes 60] {
                if {[info exists params($time)]} {
                    incr expiresIn [expr {$params($time) * $num}]
                }
            }
            if {$expiresIn != 0} {
                set secs [expr [clock seconds] + $expiresIn]
                append cookieParams "; expires=[clock_to_rfc850_gmt $secs]"
            }
        }
        if { [info exists params(path)] } {
            append cookieParams "; path=$params(path)"
        }
        if { [info exists params(domain)] } {
            append cookieParams "; domain=$params(domain)"
        }
        if { [info exists params(secure)] && $params(secure) == 1} {
            append cookieParams "; secure"
        }
        if { [info exists params(HttpOnly)] && $params(HttpOnly)} {
            append cookieParams "; HttpOnly"
        }

        return $cookieParams
    }

}
