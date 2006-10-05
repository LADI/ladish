# configure.in for omins

AC_INIT([omins],[0.2.1pre],[drobilla@connect.carleton.ca])
AC_CONFIG_SRCDIR([src/hz_voct_4200.c])

AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

AC_DISABLE_STATIC

#AC_ENABLE_STATIC(no)
#AC_ENABLE_SHARED(yes)

# Checks for compilers
AC_LANG([C])
AC_PROG_CC

AC_PROG_LIBTOOL

# Check for standard lib stuff
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h math.h])
AC_CHECK_FUNCS([strdup strndup])

# Check random other stuff
AC_C_CONST
AC_C_INLINE
AC_FUNC_MALLOC
AC_TYPE_SIZE_T

# Check for ladspa.h
AC_CHECK_HEADER(ladspa.h, , AC_MSG_ERROR([Omins requires LADSPA - you do not seem to have ladspa.h]))

# Check plugin install directory
AC_MSG_CHECKING([where to install LADSPA plugins])
AC_ARG_WITH(ladspa-dir,
	AS_HELP_STRING([--with-ladspa-dir=DIR],
		[directory that LADSPA plugins should be installed in ($libdir/ladspa)]),
	[ladspadir=$withval], [ladspadir=$libdir/ladspa])
AC_MSG_RESULT($ladspadir)
AC_SUBST(ladspadir)

# Check for debugging flag
debug="no"
AC_ARG_ENABLE(debug,
	[AS_HELP_STRING(--enable-debug, [Enable debugging (false)])],
	[debug="$enableval"])
if test "$debug" = "yes"; then
  CFLAGS="-O1 -g -DDEBUG"
else
  CFLAGS="$CFLAGS -DNDEBUG"
fi

# Check for strict flag
strict="no"
AC_ARG_ENABLE(strict,
	[AS_HELP_STRING(--enable-strict, [Enable strict compiler warnings and errors (false)])],
	[strict="$enableval"])
if test "$strict" = "yes"; then
  CFLAGS="$CFLAGS -std=c99 -pedantic -Wall -Wconversion -Winit-self"
fi

# Bolt on a few specific flags to CFLAGS that should always be used
CFLAGS="$CFLAGS -pipe -fmessage-length=139 -fdiagnostics-show-location=every-line"


AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])

AC_OUTPUT


AC_MSG_RESULT([])
AC_MSG_RESULT([**********************************************************************])
AC_MSG_RESULT([Omins build configuration:])
AC_MSG_RESULT([])
AC_MSG_RESULT([Install directory: $ladspadir])
AC_MSG_RESULT([**********************************************************************])
AC_MSG_RESULT([])

