EXTRA_DIST = reference.doxygen.in mainpage.dox

if HAVE_DOXYGEN
all: docs
else
all:
endif

reference.doxygen: @RAUL_SRCDIR@/doc/reference.doxygen.in

distclean-local:
	rm -rf html

docs: reference.doxygen @RAUL_SRCDIR@/raul/*.hpp
	doxygen reference.doxygen
