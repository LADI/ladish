AC_PREREQ(2.59)
AC_INIT([codesonnet],[svn],[drobilla@connect.carleton.ca])
AM_INIT_AUTOMAKE

AC_CONFIG_SUBDIRS([raul])
AC_CONFIG_SUBDIRS([flowcanvas])
AC_CONFIG_SUBDIRS([patchage])
AC_CONFIG_SUBDIRS([slv2])
AC_CONFIG_SUBDIRS([omins])
AC_CONFIG_SUBDIRS([ingen])
AC_CONFIG_SUBDIRS([machina])

AC_CONFIG_FILES([Makefile])

# FIXME: ok?
abs_srcdir=`pwd`

# FIXME: dirty hack for ingen engine lib dependence on raul
PKG_CHECK_MODULES(GLIBMM, glibmm-2.4)

export PKG_CONFIG_PATH="$abs_srcdir/flowcanvas:$abs_srcdir/raul:$abs_srcdir/slv2:$abs_srcdir/raul:$PKG_CONFIG_PATH"

export RAUL_CFLAGS="-I$abs_srcdir/raul $GLIBMM_CFLAGS"
export RAUL_LIBS="-L$abs_srcdir/raul/src/.libs -lraul"

export FLOWCANVAS_CFLAGS="-I$abs_srcdir/flowcanvas"
export FLOWCANVAS_LIBS="-L$abs_srcdir/flowcanvas/src/.libs -lflowcanvas"

export SLV2_CFLAGS="-I$abs_srcdir/slv2"
export SLV2_LIBS="-L$abs_srcdir/slv2/src/.libs -lslv2"

AC_MSG_NOTICE([])
AC_MSG_NOTICE([Note: PKG_CONFIG_PATH value changed to build against local libraries])
AC_MSG_NOTICE([      PKG_CONFIG_PATH = $PKG_CONFIG_PATH])
AC_MSG_NOTICE([])

AC_OUTPUT

AC_MSG_NOTICE([])
AC_MSG_NOTICE([Note: './configure --help=recursive' will show options for each package])
AC_MSG_NOTICE([      which can all be passed to this configure script])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([Now type 'make' to marvel at screenfuls of unintelligible junk])
AC_MSG_NOTICE([for what seems like days.])
