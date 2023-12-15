# -*- tcl -*-
# Tcl package index file, version 1.1
#
if {[package vsatisfies [package provide Tcl] 9.0-]} {
    package ifneeded tidy 0.3 \
	    [list load [file join $dir libtcl9tidy0.3.so] [string totitle tidy]]
} else {
    package ifneeded tidy 0.3 \
	    [list load [file join $dir libtidy0.3.so] [string totitle tidy]]
}
