EXTRA_DIST = reference.doxygen.in mainpage.dox

reference.doxygen: @REDLANDMM_SRCDIR@/doc/reference.doxygen.in

distclean-local:
	rm -rf html

docs: reference.doxygen @REDLANDMM_SRCDIR@/redlandmm/*.hpp
	doxygen reference.doxygen

