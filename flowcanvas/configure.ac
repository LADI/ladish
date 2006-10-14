AC_PREREQ(2.59)
AC_INIT([flowcanvas],[0.1.0],[drobilla@connect.carleton.ca])
AC_CONFIG_SRCDIR([src/FlowCanvas.cpp])
AC_CONFIG_SRCDIR([flowcanvas/FlowCanvas.h])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

AC_LANG([C++])

AC_PROG_CXX
AC_TYPE_SIZE_T
AC_PROG_LIBTOOL

# Check for debugging flag
debug="no"
AC_ARG_ENABLE(debug,
	[AS_HELP_STRING(--enable-debug, [Enable debugging (false)])],
	[debug="$enableval"])
if test "$debug" = "yes"; then
  # Useless POS gnomecanvasmm doesn't build w/ -pedantic
  CFLAGS="-O0 -g -DDEBUG"
  CXXFLAGS="$CFLAGS"
else
  CFLAGS="$CFLAGS -DNDEBUG"
  CXXFLAGS="$CXXFLAGS -DNDEBUG"
fi

# Check for strict flag
strict="no"
AC_ARG_ENABLE(strict,
	[AS_HELP_STRING(--enable-strict, [Enable strict compiler warnings and errors (false)])],
	[strict="$enableval"])
if test "$strict" = "yes"; then
  # Useless POS gnomecanvasmm doesn't build w/ -pedantic
  CFLAGS="$CFLAGS -ansi -Wall -Wextra -Wno-unused-parameter -Wconversion -Winit-self"
  CXXFLAGS="$CXXFLAGS -ansi -Wall -Wextra -Wno-unused-parameter -Wconversion -Winit-self -Woverloaded-virtual -Wsign-promo"
fi

# Bolt on a few specific flags to CXXFLAGS that should always be used
CXXFLAGS="$CXXFLAGS -pipe -fmessage-length=139 -fdiagnostics-show-location=every-line"
CFLAGS="$CFLAGS -pipe  -fmessage-length=139 -fdiagnostics-show-location=every-line"

# Boost shared_ptr debugging
#pointer_debug="no"
#AC_ARG_ENABLE(pointer_debug,
#	[AS_HELP_STRING(--enable-pointer-debug, [Enable smart pointer debugging (no)])],
#	[pointer_debug="$enableval"])
#if test "$pointer_debug" = "yes"; then
#  CFLAGS+=" -DBOOST_SP_ENABLE_DEBUG_HOOKS"
#  CXXFLAGS+=" -DBOOST_SP_ENABLE_DEBUG_HOOKS"
#fi

PKG_CHECK_MODULES(GTKMM, gtkmm-2.4)
PKG_CHECK_MODULES(GNOMECANVASMM, libgnomecanvasmm-2.6)

# Check for boost smart pointers
AC_CHECK_HEADERS([boost/shared_ptr.hpp], [],  
	AC_MSG_ERROR([You need the boost headers package (e.g. libboost-dev)]))
AC_CHECK_HEADERS([boost/weak_ptr.hpp], [],  
	AC_MSG_ERROR([You need the boost headers package (e.g. libboost-dev)]))

# Check for anti-aliased flag
anti_alias="no"
AC_ARG_ENABLE(anti-aliasing,
	[AS_HELP_STRING(--enable-anti-aliasing, [Enable anti aliased canvas (false, slow)])],
	[ anti_alias="$enableval"])
# Yes, this is dirty...
if test "$anti_alias" = "yes"; then
	AC_CONFIG_COMMANDS([set_aa],
		[sed 's/\(\/\*CANVASBASE\*\/\).*/\1Gnome::Canvas::CanvasAA/' -i flowcanvas/FlowCanvas.h])
else
	AC_CONFIG_COMMANDS([set_no_aa],
		[sed 's/\(\/\*CANVASBASE\*\/\).*/\1Gnome::Canvas::Canvas/' -i flowcanvas/FlowCanvas.h])
fi


AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([flowcanvas/Makefile])
AC_CONFIG_FILES([doc/Makefile])
AC_CONFIG_FILES([flowcanvas.pc])

AC_OUTPUT

AC_MSG_RESULT([])
AC_MSG_RESULT([**********************************************************************])
AC_MSG_RESULT([FlowCanvas build configuration:])
AC_MSG_RESULT([])
AC_MSG_RESULT([Anti-Aliasing: $anti_alias])
AC_MSG_RESULT([**********************************************************************])
AC_MSG_RESULT([])

