AC_PREREQ(2.59)
AC_INIT([patchage],[0.4.1],[dave@drobilla.net])

AC_CONFIG_AUX_DIR([config])


################### SOURCES

# Patchage
AC_CONFIG_SRCDIR([src/main.cpp])

AC_CONFIG_HEADER([config/config.h])

AM_INIT_AUTOMAKE

#################### ENVIRONMENT

AC_LANG([C++])

AC_PROG_CXX
AC_PROG_CC_C_O

AC_DISABLE_STATIC
AC_PROG_LIBTOOL

PKG_CHECK_MODULES(GLIBMM, glibmm-2.4)

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

PKG_CHECK_MODULES([RAUL], [raul >= 0.4.0])

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

# Jack D-Bus support
build_jack_dbus="no"
AC_ARG_ENABLE(dbus,
	[AS_HELP_STRING(--enable-dbus, [Use Jack D-Bus instead of libjack])],
	[ if test x$enable_dbus = xyes ; then build_jack_dbus=yes ; fi ])
if test "x$build_jack_dbus" = "xyes"; then
	PKG_CHECK_MODULES(DBUS, dbus-glib-1, [build_jack_dbus="yes"], [build_jack_dbus="no"])
	if test "x$build_jack_dbus" = "xyes"; then
		AC_DEFINE([HAVE_JACKDBUS], 1, [Defined if Jack D-Bus driver needs to be built])
	fi
fi
AM_CONDITIONAL(WITH_JACKDBUS, [test "$build_jack_dbus"="yes"])

	
# Jack support
build_jack="yes"
AC_ARG_ENABLE(enable-jack,
	[AS_HELP_STRING(--enable-jack, [Enable Jack support (yes)])],
	[ if test x$enable_jack = xno ; then build_jack=no ; fi ])
if test "x$build_jack" = "xyes"; then
	PKG_CHECK_MODULES(JACK, jack >= 0.107.0, [build_jack="yes"], [build_jack="no"])
	if test "x$build_jack" = "xyes"; then
		AC_DEFINE(HAVE_JACK, 1, [Has Jack])
		AC_DEFINE(HAVE_JACK_MIDI, 1, [Has Jack MIDI])
	fi
fi

# ALSA support
build_alsa="yes"
AC_ARG_ENABLE(alsa,
[AS_HELP_STRING(--enable-alsa, [Enable Alsa MIDI (yes)])],
	[ if test x$enable_alsa = xno ; then build_alsa=no ; fi ])

if test "$build_alsa" = "yes"; then
  AC_CHECK_LIB(asound, snd_seq_open, ALSA_FOUND="yes", ALSA_FOUND="no", -lm -ldl -lpthread)

  if test "$ALSA_FOUND" = "yes"; then
    AC_DEFINE(HAVE_ALSA, 1, [whether or not we have alsa])
    ALSA_CFLAGS=""
    ALSA_LIBS="-lasound -lm -ldl -lpthread"
    AC_SUBST(ALSA_CFLAGS)
    AC_SUBST(ALSA_LIBS)
  else
    build_alsa="no"
  fi
else
  ALSA_FOUND="no"
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

AM_CONDITIONAL(WITH_ALSA, [test "$ALSA_FOUND" = "yes"])
AM_CONDITIONAL(WITH_LASH, [test "$have_lash" = "yes"])
AM_CONDITIONAL(WITH_JACK, [test "$build_jack" = "yes"])
AM_CONDITIONAL(WITH_JACKDBUS, [test "$build_jack" = "dbus"])

############# PATCHAGE
PKG_CHECK_MODULES(GTHREAD, gthread-2.0)
PKG_CHECK_MODULES(GTKMM, gtkmm-2.4)
PKG_CHECK_MODULES(GNOMECANVASMM, libgnomecanvasmm-2.6)
PKG_CHECK_MODULES(LIBGLADEMM, libglademm-2.4)
PKG_CHECK_MODULES(FLOWCANVAS, flowcanvas >= 0.4.0)


#################### OUTPUT

# Patchage
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([patchage.desktop])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([icons/Makefile])
AC_CONFIG_FILES([icons/16x16/Makefile])
AC_CONFIG_FILES([icons/22x22/Makefile])
AC_CONFIG_FILES([icons/24x24/Makefile])
AC_CONFIG_FILES([icons/32x32/Makefile])
AC_CONFIG_FILES([icons/48x48/Makefile])
AC_CONFIG_FILES([icons/scalable/Makefile])

AC_OUTPUT

AC_MSG_RESULT([])
AC_MSG_RESULT([])
AC_MSG_RESULT([**********************************************************************])
AC_MSG_RESULT([Configuration:])
AC_MSG_RESULT([])
AC_MSG_RESULT([Jack support:            $build_jack])
AC_MSG_RESULT([Jack D-Bus:              $build_jack_dbus])
AC_MSG_RESULT([LASH support:            $build_lash])
AC_MSG_RESULT([ALSA support:            $ALSA_FOUND])
AC_MSG_RESULT([])
AC_MSG_RESULT([C FLAGS:   $CFLAGS])
AC_MSG_RESULT([C++ FLAGS: $CXXFLAGS])
AC_MSG_RESULT([Please note the above and see README for performance information.])
AC_MSG_RESULT([**********************************************************************])
AC_MSG_RESULT([])

