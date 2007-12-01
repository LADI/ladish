EXTRA_DIST = reference.doxygen.in mainpage.dox

reference.doxygen: @FLOWCANVAS_SRCDIR@/doc/reference.doxygen.in

distclean-local:
	rm -rf html

docs: reference.doxygen @FLOWCANVAS_SRCDIR@/flowcanvas/*.hpp
	doxygen reference.doxygen
