# configure.in for machina

AC_INIT(machina, 0.0.1, dave@drobilla.net)
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_SRCDIR([src/engine/Machine.cpp])
AC_CONFIG_SRCDIR([src/engine/machina/Machine.hpp])
AC_CONFIG_SRCDIR([src/gui/main.cpp])
#AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

AC_LANG([C++])

AC_PROG_CXX
AC_PROG_LIBTOOL

AC_TYPE_SIZE_T

PKG_CHECK_MODULES(JACK, jack >= 0.102.20)
PKG_CHECK_MODULES(RAUL, raul >= 0.0.0)
PKG_CHECK_MODULES(GLIBMM, glibmm-2.4)

# Check for debugging flag
debug="no"
AC_ARG_ENABLE(debug,
	[AS_HELP_STRING(--enable-debug, [Enable debugging (no)])],
	[debug="$enableval"])
if test "$debug" = "yes"; then
  # Useless POS gnomecanvasmm doesn't build w/ -pedantic
  CFLAGS="-O0 -g -DDEBUG"
  CXXFLAGS="$CFLAGS"
else
  CFLAGS="$CFLAGS -DNDEBUG"
  CXXFLAGS="$CFLAGS -DNDEBUG"
fi

# Check for strict flag
strict="no"
AC_ARG_ENABLE(strict,
	[AS_HELP_STRING(--enable-strict, [Enable strict compiler warnings or errors (no)])],
	[strict="$enableval"])
if test "$strict" = "yes"; then
  # Useless POS gnomecanvasmm doesn't build w/ -pedantic
  CFLAGS="$CFLAGS -ansi -Wall -Wextra -Wno-unused-parameter -Wconversion -Winit-self"
  CXXFLAGS="$CXXFLAGS -ansi -Wall -Wextra -Wno-unused-parameter -Wconversion -Winit-self -Woverloaded-virtual -Wsign-promo"
fi

# Boost shared_ptr debugging
pointer_debug="no"
AC_ARG_ENABLE(pointer_debug,
	[AS_HELP_STRING(--enable-pointer-debug, [Enable smart pointer debugging (no)])],
	[pointer_debug="$enableval"])
if test "$pointer_debug" = "yes"; then
  CFLAGS+=" -DBOOST_SP_ENABLE_DEBUG_HOOKS"
  CXXFLAGS+=" -DBOOST_SP_ENABLE_DEBUG_HOOKS"
fi

# Build GUI?
build_gui="yes"
AC_ARG_ENABLE(gui,
	[AS_HELP_STRING(--enable-gui, [Build GUI (yes)])],
	[build_gui="$enableval"])
if test "$build_gui" = "yes"; then
	PKG_CHECK_MODULES(GTKMM, gtkmm-2.4)
	PKG_CHECK_MODULES(GNOMECANVASMM, libgnomecanvasmm-2.6)
	PKG_CHECK_MODULES(LIBGLADEMM, libglademm-2.4)
	PKG_CHECK_MODULES(FLOWCANVAS, flowcanvas >= 0.1.0)
fi
AM_CONDITIONAL(WITH_GUI, [test "x$build_gui" = "xyes"])

# Bolt on a few specific flags to CXXFLAGS that should always be used
CXXFLAGS="$CXXFLAGS -pipe -Wall -fmessage-length=139 -fdiagnostics-show-location=every-line"
CFLAGS="$CFLAGS -pipe -Wall -fmessage-length=139 -fdiagnostics-show-location=every-line"

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([src/engine/Makefile])
AC_CONFIG_FILES([src/engine/machina/Makefile])
AC_CONFIG_FILES([src/gui/Makefile])
AC_CONFIG_FILES([util/Makefile])
AC_CONFIG_FILES([machina.desktop])

AC_OUTPUT

AC_MSG_RESULT([])
AC_MSG_RESULT([**********************************************************************])
AC_MSG_RESULT([Machina build configuration:])
AC_MSG_RESULT([])
AC_MSG_RESULT([Super whiz-bang 2.0 Plus:  oh yes])
AC_MSG_RESULT([**********************************************************************])
AC_MSG_RESULT([])

