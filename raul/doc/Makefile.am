EXTRA_DIST = reference.doxygen mainpage.dox

docs: @RAUL_SRCDIR@/doc/reference.doxygen @RAUL_SRCDIR@/raul/*.hpp
	mkdir -p @abs_top_builddir@/doc/raul
	doxygen @RAUL_SRCDIR@/doc/reference.doxygen
