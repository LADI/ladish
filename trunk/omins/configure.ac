AC_PREREQ(2.59)
AC_INIT([drobillad],[svn],[dave@drobilla.net])

AC_CONFIG_AUX_DIR([config])

AC_CONFIG_SRCDIR([src/hz_voct_4200.c])

AM_INIT_AUTOMAKE

#################### ENVIRONMENT

AC_LANG([C++])

AC_PROG_CXX
AM_PROG_CC_C_O

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
  CFLAGS="$CFLAGS -DDEBUG"
  CXXFLAGS="$CXXFLAGS -DDEBUG"
else
  CFLAGS="$CFLAGS -DNDEBUG"
  CXXFLAGS="$CXXFLAGS -DNDEBUG"
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

AC_CHECK_HEADER([ladspa.h], [build_ladspa="yes"],
	[AC_MSG_WARN([Omins requires ladspa.h])])

AC_DEFINE(HAVE_LADSPA, 1, [Has ladspa.h])

AM_CONDITIONAL(WITH_LADSPA, [test "x$build_ladspa" = "xyes"])


####################### OMINS

# Check plugin install directory
AC_MSG_CHECKING([where to install LADSPA plugins])
AC_ARG_WITH(ladspa-dir,
	AS_HELP_STRING([--with-ladspa-dir=DIR],
		[directory where LADSPA plugins should be installed ($libdir/ladspa)]),
	[ladspadir=$withval], [ladspadir=$libdir/ladspa])
AC_MSG_RESULT($ladspadir)
AC_SUBST(ladspadir)

# Doxygen is required to make dist
AC_CHECK_PROG(HAVE_DOXYGEN, doxygen, true, false)
if test $HAVE_DOXYGEN = "false"; then
	AC_MSG_WARN([Doxygen not found, documentation will not be built])
fi
AM_CONDITIONAL(HAVE_DOXYGEN, $HAVE_DOXYGEN)


#################### OUTPUT

# Omins
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])

AC_OUTPUT

AC_MSG_RESULT([])
AC_MSG_RESULT([])
AC_MSG_RESULT([**********************************************************************])
AC_MSG_RESULT([Configuration:])
AC_MSG_RESULT([])
AC_MSG_RESULT([LADSPA plugin install location: $ladspadir])
AC_MSG_RESULT([])
AC_MSG_RESULT([C FLAGS:   $CFLAGS])
AC_MSG_RESULT([C++ FLAGS: $CXXFLAGS])
AC_MSG_RESULT([**********************************************************************])
AC_MSG_RESULT([])

