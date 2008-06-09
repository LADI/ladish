AC_PREREQ(2.59)
AC_INIT([flowcanvas],[0.4.1],[dave@drobilla.net])

AC_CONFIG_AUX_DIR([config])


################### SOURCES

# FlowCanvas
AC_CONFIG_SRCDIR([src/Canvas.cpp])
AC_CONFIG_SRCDIR([flowcanvas/Canvas.hpp])

AC_CONFIG_HEADER([config/config.h])

AM_INIT_AUTOMAKE

# FlowCanvas interface version (libtool shared library versioning)
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
# 0.4.1 = 1,0,0

FLOWCANVAS_VERSION="0.4.1"
FLOWCANVAS_API_CURRENT="1"
FLOWCANVAS_API_REVISION="0"
FLOWCANVAS_API_AGE="0"

AC_SUBST(FLOWCANVAS_API_CURRENT)
AC_SUBST(FLOWCANVAS_API_REVISION)
AC_SUBST(FLOWCANVAS_API_AGE)
FLOWCANVAS_SO_VERSION="${FLOWCANVAS_API_CURRENT}:${FLOWCANVAS_API_REVISION}:${FLOWCANVAS_API_AGE}"
AC_SUBST(FLOWCANVAS_SO_VERSION)
AC_SUBST(FLOWCANVAS_VERSION)


#################### ENVIRONMENT

AC_LANG([C++])

AC_PROG_CXX
AC_PROG_CC_C_O

AC_DISABLE_STATIC
AC_PROG_LIBTOOL

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

FLOWCANVAS_CFLAGS="-I$abs_srcdir"
FLOWCANVAS_LIBS="$builddir/src/.libs/libflowcanvas.la"
FLOWCANVAS_SRCDIR="$abs_srcdir"
AC_SUBST(FLOWCANVAS_CFLAGS)
AC_SUBST(FLOWCANVAS_LIBS)
AC_SUBST(FLOWCANVAS_SRCDIR)

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

AC_ARG_ENABLE(debug_asertions,
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

# (none apply)

########### FLOWCANVAS

PKG_CHECK_MODULES(GTKMM, gtkmm-2.4)
PKG_CHECK_MODULES(GNOMECANVASMM, libgnomecanvasmm-2.6)

# Check for anti-aliased flag
anti_alias="yes"
AC_ARG_ENABLE(anti-aliasing,
	[AS_HELP_STRING(--enable-anti-aliasing, [Enable anti aliased canvas (yes)])],
	[ anti_alias="$enableval"])
# Yes, this is dirty...
if test "$anti_alias" = "yes"; then
	AC_CONFIG_COMMANDS([set_aa],
		[sed 's/\(\/\*CANVASBASE\*\/\).*/\1Gnome::Canvas::CanvasAA/' -i $srcdir/flowcanvas/Canvas.hpp])
else
	AC_CONFIG_COMMANDS([set_no_aa],
		[sed 's/\(\/\*CANVASBASE\*\/\).*/\1Gnome::Canvas::Canvas/' -i $srcdir/flowcanvas/Canvas.hpp])
fi

# Check for the graphviz library for auto-layout support
graphviz="no"
AC_ARG_ENABLE(graphviz,
	[AS_HELP_STRING(--enable-graphviz, [Use graphviz to automatically organize canvas (yes)])],
	[ graphviz="$enableval"])
PKG_CHECK_MODULES(AGRAPH, libgvc >= 2.8, graphviz="yes", graphviz="no")
if test "$graphviz" = "yes"; then
	AC_DEFINE(HAVE_AGRAPH, 1, [Has the graphviz library])
else
	AC_MSG_WARN([Warning: Graphviz 'agraph' library (graphviz-dev) not found, no auto-arrange support
				 will be built])
fi

# Build documentation
build_documentation="yes"
AC_ARG_ENABLE(documentation,
	[AS_HELP_STRING(--enable-documentation, [Build documentation (no)])],
	[build_documentation="$enableval"])
AC_CHECK_PROG(HAVE_DOXYGEN, doxygen, true, false)
if test $HAVE_DOXYGEN = "false"; then
	AC_MSG_WARN([*** Doxygen not found, API documentation will not be built])
	build_documentation="no"
fi
AM_CONDITIONAL(HAVE_DOXYGEN, [test "x$build_documentation" = "xyes"])


#################### OUTPUT

# FlowCanvas
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([flowcanvas/Makefile])
AC_CONFIG_FILES([doc/Makefile])
AC_CONFIG_FILES([doc/reference.doxygen])
AC_CONFIG_FILES([flowcanvas.pc])

AC_OUTPUT

AC_MSG_RESULT([])
AC_MSG_RESULT([])
AC_MSG_RESULT([**********************************************************************])
AC_MSG_RESULT([Configuration:])
AC_MSG_RESULT([])
AC_MSG_RESULT([Anti-Aliasing:               $anti_alias])
AC_MSG_RESULT([Auto-Arrange (via GraphViz): $graphviz])
AC_MSG_RESULT([])
AC_MSG_RESULT([C FLAGS:   $CFLAGS])
AC_MSG_RESULT([C++ FLAGS: $CXXFLAGS])
AC_MSG_RESULT([Please note the above and see README for performance information.])
AC_MSG_RESULT([**********************************************************************])
AC_MSG_RESULT([])

