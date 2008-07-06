AC_PREREQ(2.59)
AC_INIT([raul],[0.5.0],[dave@drobilla.net])

AC_CONFIG_AUX_DIR([config])


################### SOURCES

# Raul
AC_CONFIG_SRCDIR([raul/Semaphore.h])
AC_CONFIG_SRCDIR([src/Thread.cpp])

AC_CONFIG_HEADER([config/config.h])

AM_INIT_AUTOMAKE

# Raul interface version (libtool shared library versioning)
#
# current = incremented whenever the public API is changed 
# revision = incremented when the implementation of current is changed
# age = current library is both source and binary compatible with
#       interfaces: current,current-1,...,current-age
#
# See libtool documentation for detailed documentation
#
# Version history:
# 0.4.0 = 0,0,0
# 0.5.0 = 1,0,0 (SVN r1283)

RAUL_VERSION="0.5.0"
RAUL_API_CURRENT="1"
RAUL_API_REVISION="0"
RAUL_API_AGE="0"

AC_SUBST(RAUL_API_CURRENT)
AC_SUBST(RAUL_API_REVISION)
AC_SUBST(RAUL_API_AGE)
RAUL_SO_VERSION="${RAUL_API_CURRENT}:${RAUL_API_REVISION}:${RAUL_API_AGE}"
AC_SUBST(RAUL_SO_VERSION)
AC_SUBST(RAUL_VERSION)


#################### ENVIRONMENT

AC_LANG([C++])

AC_PROG_CXX

AC_DISABLE_STATIC
AC_PROG_LIBTOOL

PKG_CHECK_MODULES(GLIBMM, glibmm-2.4)
PKG_CHECK_MODULES(GTHREAD, gthread-2.0)

# Check for boost smart pointers
AC_CHECK_HEADERS([boost/shared_ptr.hpp], [],  
	AC_MSG_ERROR([You need the boost headers package (e.g. libboost-dev)]))
AC_CHECK_HEADERS([boost/weak_ptr.hpp], [],  
	AC_MSG_ERROR([You need the boost headers package (e.g. libboost-dev)]))

# This is cute... autohell FTW
builddir=`pwd`
cd $srcdir
abs_srcdir=`pwd`
cd $builddir

RAUL_CFLAGS="-I$abs_srcdir"
RAUL_LIBS="$builddir/src/.libs/libraul.la"
RAUL_SRCDIR="$abs_srcdir"
AC_SUBST(RAUL_CFLAGS)
AC_SUBST(RAUL_LIBS)
AC_SUBST(RAUL_SRCDIR)

CONFIG_H_PATH="$builddir/config/config.h"

#################### COMMAND LINE PARAMETERS

AC_ARG_ENABLE(debug,
	[AS_HELP_STRING(--enable-debug, [Enable debugging symbols and assertions (no)])],
	[debug="$enableval"])
if test "$debug" = "yes"; then
  debug_symbols="yes"
  debug_assertions="yes"
fi

AC_ARG_ENABLE(debug_symbols,
	[AS_HELP_STRING(--enable-debug-symbols, [Enable debugging symbols - overrides CXXFLAGS (no)])],
	[debug_symbols="$enableval"])

AC_ARG_ENABLE(debug_assertions,
	[AS_HELP_STRING(--enable-debug-assertions, [Enable debugging assertions (no)])],
	[debug_assertions="$enableval"])

if test "$debug_symbols" = "yes"; then
  CFLAGS="-O0 -g"
  CXXFLAGS="-O0 -g"
fi

if test "$debug_assertions" = "yes"; then
  CFLAGS="$CFLAGS -DDEBUG -DLIBRDF_DEBUG"
  CXXFLAGS="$CXXFLAGS -DDEBUG -DLIBRDF_DEBUG"
else
  CFLAGS="$CFLAGS -DNDEBUG"
  CXXFLAGS="$CXXFLAGS -DNDEBUG"
fi

# Boost shared_ptr debugging
pointer_debug="no"
AC_ARG_ENABLE(pointer_debug,
	[AS_HELP_STRING(--enable-pointer-debug, [Enable smart pointer debugging (no)])],
	[pointer_debug="$enableval"])
if test "$pointer_debug" = "yes"; then
  CFLAGS="$CFLAGS -DBOOST_SP_ENABLE_DEBUG_HOOKS"
  CXXFLAGS="$CXXFLAGS -DBOOST_SP_ENABLE_DEBUG_HOOKS"
fi

# Use strict flags?
strict="no"
AC_ARG_ENABLE(strict,
	[AS_HELP_STRING(--enable-strict, [Enable strict compiler warnings and errors (no)])],
	[strict="$enableval"])
if test "$strict" = "yes"; then
  # Stupid Gtkmm won't build with -pedantic
  CFLAGS="$CFLAGS -Wall -Wextra -Wno-unused-parameter -Winit-self"
  CXXFLAGS="$CXXFLAGS -Wall -Wextra -Wno-unused-parameter -Winit-self -Woverloaded-virtual -Wsign-promo"
fi

# Bolt on a few specific flags to CFLAGS that should always be used
CXXFLAGS="$CXXFLAGS -ansi -pipe -fmessage-length=999 -DCONFIG_H_PATH=\\\"$CONFIG_H_PATH\\\""
CFLAGS="$CFLAGS -std=c99 -pipe -fmessage-length=999 -DCONFIG_H_PATH=\\\"$CONFIG_H_PATH\\\""


#################### GENERIC OPTIONS


# Jack support
build_jack="yes"
AC_ARG_ENABLE(enable-jack,
	[AS_HELP_STRING(--enable-jack, [Enable Jack support (yes)])],
	[ if test x$enable_jack = xno ; then build_jack=no ; fi ])
if test "$build_jack" = "yes"; then
	PKG_CHECK_MODULES(JACK, jack >= 0.107.0)
	AC_DEFINE(HAVE_JACK_MIDI, 1, [Has Jack MIDI])
fi

# LASH support
build_lash="yes"
AC_ARG_ENABLE(lash,
	[AS_HELP_STRING(--enable-lash, [Enable LASH session management support (yes)])],
	[ if test x$enable_lash = xno ; then build_lash=no ; fi ])
	
have_lash="no"
if test "$build_lash" = "yes"; then
	PKG_CHECK_MODULES(LASH, lash-1.0 >= 0.5.2, have_lash="yes", have_lash="no")
fi
if test "$have_lash" = "yes"; then
	AC_DEFINE(HAVE_LASH, 1, [Has lash.h])
else
	AC_MSG_WARN([LASH not found, session support will not be built.])
fi

# Build documentation
build_documentation="yes"
AC_ARG_ENABLE(documentation,
	[AS_HELP_STRING(--enable-documentation, [Build documentation (yes, if doxygen available)])],
	[build_documentation="$enableval"])
if test "$build_documentation" = "yes"; then
	AC_CHECK_PROG(HAVE_DOXYGEN, doxygen, true, false)
	if test $HAVE_DOXYGEN = "false"; then
		AC_MSG_WARN([*** Doxygen not found, API documentation will not be built])
		build_documentation="no"
	fi
fi
AM_CONDITIONAL(HAVE_DOXYGEN, [test "x$build_documentation" = "xyes"])

# Build unit tests
build_unit_tests="no"
AC_ARG_ENABLE(unit-tests,
	[AS_HELP_STRING(--enable-unit-tests, [Build unit tests (no) - Developers only])],
	[build_unit_tests="$enableval"])

AM_CONDITIONAL(WITH_JACK, [test "$build_jack" = "yes"])
AM_CONDITIONAL(WITH_LASH, [test "$have_lash" = "yes"])
AM_CONDITIONAL(BUILD_UNIT_TESTS, [test "$build_unit_tests" = "yes"])


################## RAUL

# (No required dependencies)

#################### OUTPUT

# RAUL
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([debian/Makefile])
AC_CONFIG_FILES([doc/Makefile])
AC_CONFIG_FILES([doc/reference.doxygen])
AC_CONFIG_FILES([raul.pc])
AC_CONFIG_FILES([raul/Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([tests/Makefile])

AC_OUTPUT

AC_MSG_RESULT([])
AC_MSG_RESULT([])
AC_MSG_RESULT([**********************************************************************])
AC_MSG_RESULT([Configuration:])
AC_MSG_RESULT([])
AC_MSG_RESULT([Jack support:     $build_jack])
AC_MSG_RESULT([LASH support:     $build_lash])
AC_MSG_RESULT([Build unit tests: $build_unit_tests])
AC_MSG_RESULT([])
AC_MSG_RESULT([C FLAGS:   $CFLAGS])
AC_MSG_RESULT([C++ FLAGS: $CXXFLAGS])
AC_MSG_RESULT([**********************************************************************])
AC_MSG_RESULT([])


