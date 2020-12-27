tcl-tidy
=====

This package is Tcl bindings for libtidy.

[libtidy](https://www.html-tidy.org/developer//) is the library version of HTML Tidy.
Libtidy provides [API and Quick Reference](http://api.html-tidy.org/#part_apiref).


License
=====

tcl-tidy is Licensed under the MIT license.


Implement commands
=====

tidy::tidy   
tidy::libversion  

TIDYDOC quick_repair string encoding_name  
TIDYDOC parse_string string encoding_name  
TIDYDOC clean_repair  
TIDYDOC diagnose  
TIDYDOC get_output  
TIDYDOC get_status  
TIDYDOC get_config  
TIDYDOC set_config options_list  
TIDYDOC get_opt option_name  
TIDYDOC set_opt option_name option_value  
TIDYDOC error_count  
TIDYDOC warning_count  
TIDYDOC access_count  
TIDYDOC close  


UNIX BUILD
=====

I only test tcl-tidy under openSUSE LEAP 15.2.

Users need install libtidy development files.
Below is an example for openSUSE:

        sudo zypper in libtidy-devel

Building under most UNIX systems is easy, just run the configure script
and then run make. For more information about the build process, see the
tcl/unix/README file in the Tcl src dist. The following minimal example
will install the extension in the /opt/tcl directory.

        $ cd tcl-tidy
        $ ./configure --prefix=/opt/tcl
        $ make
        $ make install

If you need setup directory containing tcl configuration (tclConfig.sh),
below is an example:

        $ cd tcl-tidy
        $ ./configure --with-tcl=/opt/activetcl/lib
        $ make
        $ make install


Example
=====

Below is a simple HTML example:

        package require tidy
        set handle [tidy::tidy create]
        $handle parse_string "<p>Hello" utf8
        $handle clean_repair
        $handle set_config [list force-output 1 show-body-only 1]
        puts [$handle get_output]
        $handle close

Below is a simple XML example:

        package require tidy
        set handle [tidy::tidy create]
        $handle set_opt input-xml 1
        $handle parse_string "<HELLO>World" utf8
        $handle clean_repair
        puts [$handle get_output]
        $handle close

