AC_PREREQ(2.59)
AC_INIT([raul],[svn],[dave@drobilla.net])

AC_CONFIG_AUX_DIR([config])


################### SOURCES

# Raul
AC_CONFIG_SRCDIR([raul/Semaphore.h])
AC_CONFIG_SRCDIR([src/Thread.cpp])

AC_CONFIG_HEADER([config/config.h])

AM_INIT_AUTOMAKE

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
AC_SUBST(RAUL_CFLAGS)
AC_SUBST(RAUL_LIBS)

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
	[assertions="$enableval"])

if test "$debug_symbols" = "yes"; then
  CFLAGS="-O0 -g"
  CXXFLAGS="-O0 -g"
fi

if test "$debug_assertions" = "yes"; then
  CFLAGS="$CFLAGS -DDEBUG -DLIBRDF_DEBUG"
  CXXFLAGS="$CXXFLAGS -DDEBUG -DLIBRDF_DEBUG"
else
  CFLAGS="$CFLAGS -DNDEBUG -DLIBRDF_DEBUG"
  CXXFLAGS="$CXXFLAGS -DNDEBUG -DLIBRDF_DEBUG"
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

# Build unit tests
build_unit_tests="no"
AC_ARG_ENABLE(unit-tests,
	[AS_HELP_STRING(--enable-unit-tests, [Build unit tests (no) - Developers only])],
	[build_unit_tests="$enableval"])

AM_CONDITIONAL(WITH_JACK, [test "$build_jack" = "yes"])
AM_CONDITIONAL(WITH_LASH, [test "$have_lash" = "yes"])
AM_CONDITIONAL(BUILD_UNIT_TESTS, [test "$build_unit_tests" = "yes"])


################## RAUL

# Check for Redlandmm
build_redlandmm="no"
PKG_CHECK_MODULES([REDLANDMM], [redlandmm], [build_redlandmm="yes"], [build_redlandmm="no"])
AM_CONDITIONAL(WITH_REDLANDMM, [test "$build_redlandmm" = "yes"])
AC_DEFINE(HAVE_REDLANDMM, 1, [Has redlandmm])

# Check for liblo
build_liblo="yes"
AC_ARG_ENABLE(liblo,
	[AS_HELP_STRING(--enable-liblo, [Include liblo (OSC) support (yes)])],
	[ if test x$enable_liblo = xno ; then build_liblo=no ; fi ])
if test "$build_liblo" = "yes"; then
	PKG_CHECK_MODULES(LIBLO, liblo >= 0.22, [],
		AC_MSG_ERROR([OSC support requires liblo (>= 0.22)]))
fi
AM_CONDITIONAL(WITH_LIBLO, [test "$build_liblo" = "yes"])


#################### OUTPUT

# RAUL
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([raul/Makefile])
AC_CONFIG_FILES([tests/Makefile])
AC_CONFIG_FILES([doc/Makefile])
AC_CONFIG_FILES([doc/reference.doxygen])
AC_CONFIG_FILES([raul.pc])

AC_OUTPUT

AC_MSG_RESULT([])
AC_MSG_RESULT([])
AC_MSG_RESULT([**********************************************************************])
AC_MSG_RESULT([Configuration:])
AC_MSG_RESULT([])
AC_MSG_RESULT([Jack support:     $build_jack])
AC_MSG_RESULT([LASH support:     $build_lash])
AC_MSG_RESULT([RDF support:      $build_redlandmm])
AC_MSG_RESULT([Build unit tests: $build_unit_tests])
AC_MSG_RESULT([])
AC_MSG_RESULT([C FLAGS:   $CFLAGS])
AC_MSG_RESULT([C++ FLAGS: $CXXFLAGS])
AC_MSG_RESULT([**********************************************************************])
AC_MSG_RESULT([])


