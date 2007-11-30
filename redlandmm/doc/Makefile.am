EXTRA_DIST = reference.doxygen mainpage.dox

docs: @REDLANDMM_SRCDIR@/doc/reference.doxygen @REDLANDMM_SRCDIR@/redlandmm/*.hpp
	mkdir -p @abs_top_builddir@/doc/redlandmm
	doxygen @REDLANDMM_SRCDIR@/doc/reference.doxygen
