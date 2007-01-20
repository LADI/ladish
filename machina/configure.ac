# configure.in for machina

AC_INIT(machina, 0.0.1, dave@codeson.net)
AC_CONFIG_SRCDIR([src/main.cpp])
#AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

AC_LANG([C++])

AC_PROG_CXX
AC_TYPE_SIZE_T

PKG_CHECK_MODULES(JACK, jack >= 0.102.20)
PKG_CHECK_MODULES(RAUL, raul >= 0.0.0)

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

# Bolt on a few specific flags to CXXFLAGS that should always be used
CXXFLAGS="$CXXFLAGS -pipe -Wall -fmessage-length=139 -fdiagnostics-show-location=every-line"
CFLAGS="$CFLAGS -pipe -Wall -fmessage-length=139 -fdiagnostics-show-location=every-line"

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([machina.desktop])

AC_OUTPUT

#AC_MSG_RESULT([])
#AC_MSG_RESULT([**********************************************************************])
#AC_MSG_RESULT([Patchage build configuration:])
#AC_MSG_RESULT([])
#AC_MSG_RESULT([Alsa support: $build_alsa])
#AC_MSG_RESULT([Lash support: $have_lash])
#AC_MSG_RESULT([**********************************************************************])
#AC_MSG_RESULT([])

