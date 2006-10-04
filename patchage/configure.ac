# configure.in for patchage

AC_INIT(patchage, 0.2.4pre, drobilla@connect.carleton.ca)
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

# Checks for compilers
AC_PROG_CXX

# Check other stuff..
AC_TYPE_SIZE_T
AC_CHECK_FUNCS([strerror])

# Check for pthreads
AC_CHECK_LIB(pthread, pthread_create, [],
	AC_MSG_ERROR([*** Patchage requires POSIX threads support]))
AC_CHECK_HEADER([pthread.h])

PKG_CHECK_MODULES(RAUL, raul >= 0.0.0)

# Check for FlowCanvas
PKG_CHECK_MODULES(FLOWCANVAS, flowcanvas >= 0.1.0)
AC_SUBST(FLOWCANVAS_CFLAGS)
AC_SUBST(FLOWCANVAS_LIBS)

# Check for Jack
PKG_CHECK_MODULES(JACK, jack >= 0.100.0)

# Check for ALSA
enable_alsa="yes"
AC_ARG_ENABLE(alsa,
[AS_HELP_STRING(--enable-alsa, [Enable Alsa MIDI driver (yes)])],
[ enable_alsa="$enableval" ]
)

if test "$enable_alsa" = "yes"; then
  AC_CHECK_LIB(asound, snd_seq_open, ALSA_FOUND="yes", ALSA_FOUND="no", -lm -ldl -lpthread)

  if test "$ALSA_FOUND" = "yes"; then
    AC_DEFINE(HAVE_ALSA, 1, [whether or not we have alsa])
    ALSA_CFLAGS=""
    ALSA_LIBS="-lasound -lm -ldl -lpthread"
    AC_SUBST(ALSA_CFLAGS)
    AC_SUBST(ALSA_LIBS)
  fi
else
  ALSA_FOUND="no"
fi
AM_CONDITIONAL(ALSA, test "$ALSA_FOUND" = "yes")

# LASH config option
build_lash="yes"
AC_ARG_ENABLE(lash,
	[AS_HELP_STRING(--enable-lash, [Enable LASH session management support (yes)])],
	[ if test x$enable_lash = xno ; then build_lash=no ; fi ])
	
have_lash="no"
if test "$build_lash" = "yes"; then
	PKG_CHECK_MODULES(LASH, lash-1.0 >= 0.5.0, have_lash="yes", have_lash="no")
fi
if test "$have_lash" = "yes"; then
	AC_DEFINE(HAVE_LASH, 1, [Has lash.h])
	AC_SUBST(LASH_CFLAGS)
	AC_SUBST(LASH_LIBS)
else
	AC_MSG_WARN([LASH not found, session support will not be built.])
fi
AM_CONDITIONAL(WITH_LASH, [test "$have_lash" = "yes"])


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

# Bolt on a few specific flags to CXXFLAGS that should always be used
CXXFLAGS="$CXXFLAGS -pipe -Wall -fmessage-length=139 -fdiagnostics-show-location=every-line"
CFLAGS="$CFLAGS -pipe -Wall -fmessage-length=139 -fdiagnostics-show-location=every-line"

# Check for GTKMM
PKG_CHECK_MODULES(GTKMM, gtkmm-2.4)
AC_SUBST(GTKMM_CFLAGS)
AC_SUBST(GTKMM_LIBS)

# Check for gnomecanvasmm
PKG_CHECK_MODULES(GNOMECANVASMM, libgnomecanvasmm-2.6)
AC_SUBST(GNOMECANVASMM_CFLAGS)
AC_SUBST(GNOMECANVASMM_LIBS)

# Check for libglademm
PKG_CHECK_MODULES(LIBGLADEMM, libglademm-2.4)
AC_SUBST(LIBGLADEMM_CFLAGS)
AC_SUBST(LIBGLADEMM_LIBS)

# Write it!
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_OUTPUT
