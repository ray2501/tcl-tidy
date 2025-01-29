# -*- tcl -*-
# Tcl package index file, version 1.1
#
if {[package vsatisfies [package provide Tcl] 9.0-]} {
    package ifneeded tidy 0.4 \
	    [list load [file join $dir libtcl9tidy0.4.so] [string totitle tidy]]
} else {
    package ifneeded tidy 0.4 \
	    [list load [file join $dir libtidy0.4.so] [string totitle tidy]]
}
