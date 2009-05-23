AC_INIT([LASH],[0.6.0])
AC_CONFIG_SRCDIR([lash/types.h])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE
#AM_ACLOCAL_INCLUDE([m4])


### Check for programs ###
AC_LANG([C])
AM_PROG_CC_C_O
AC_PROG_LIBTOOL


######################
### --enable-debug ###
######################

lash_debug="no"
AC_MSG_CHECKING([whether to enable debugging output])
AC_ARG_ENABLE(debug,
  [AS_HELP_STRING(--enable-debug, enable debugging output [default=no])],
  [case "$enableval" in
    "yes")
      lash_debug="yes"
      ;;
    "no")
      ;;
    *)
      AC_MSG_ERROR([must use --enable-debug(=yes/no) or --disable-debug])
      ;;
  esac]
)
if test "x$lash_debug" = "xyes"; then
  AC_MSG_RESULT([yes])
  AC_DEFINE(LASH_DEBUG, 1, [whether or not to enable debugging output])
else
  AC_MSG_RESULT([no])
fi


########################
### --enable-old-api ###
########################

lash_old_api="yes"
AC_MSG_CHECKING([whether to enable the old API])
AC_ARG_ENABLE(old-api,
  [AS_HELP_STRING(--enable-old-api, enable the old API [default=yes])],
  [case "$enableval" in
    "yes")
      ;;
    "no")
      lash_old_api="no"
      ;;
    *)
      AC_MSG_ERROR([must use --enable-old-api(=yes/no) or --disable-old-api])
      ;;
  esac]
)
if test "x$lash_old_api" = "xyes"; then
  AC_MSG_RESULT([yes])
else
  AC_MSG_RESULT([no])
fi


########################
### --with-jack-dbus ###
########################

lash_jack_dbus="yes"
AC_MSG_CHECKING([whether to use D-Bus for JACK communication])
AC_ARG_WITH(jack-dbus, [
AS_HELP_STRING(--with-jack-dbus, [use D-Bus for JACK communication [default=yes]])],
  [case "x$withval" in
    "xyes")
      ;;
    "xno")
      lash_jack_dbus="no"
      ;;
    *)
      AC_MSG_ERROR([must use --with-jack-dbus(=yes/no) or --without-jack-dbus])
      ;;
  esac]
)
if test "x$lash_jack_dbus" = "xyes"; then
  AC_MSG_RESULT([yes])
else
  AC_MSG_RESULT([no])
fi


###################
### --with-alsa ###
###################

lash_alsa=""
AC_MSG_CHECKING([whether to build with ALSA MIDI support])
AC_ARG_WITH(alsa,
  [AS_HELP_STRING(--with-alsa, [build with ALSA MIDI support [default=auto]])],
  [case "x$withval" in
    "xyes")
      lash_alsa="yes"
      ;;
    "xno")
      lash_alsa="no"
      ;;
    *)
      AC_MSG_ERROR([must use --with-alsa(=yes/no) or --without-alsa])
      ;;
  esac]
)
if test "x$lash_alsa" = "x"; then
  AC_MSG_RESULT([auto])
elif test "x$lash_alsa" = "xyes"; then
  AC_MSG_RESULT([yes])
else
  AC_MSG_RESULT([no])
fi


###################
### --with-gtk2 ###
###################

lash_gtk2=""
AC_MSG_CHECKING([whether to build clients which require GTK+ 2])
AC_ARG_WITH(gtk2,
  [AS_HELP_STRING(--with-gtk2, [build clients which require GTK+ 2 [default=auto]])],
  [case "$withval" in
    "yes")
      lash_gtk2="yes"
      ;;
    "no")
      lash_gtk2="no"
      ;;
    *)
      AC_MSG_ERROR([must use --with-gtk2(=yes/no) or --without-gtk2])
      ;;
  esac]
)
if test "x$lash_gtk2" = "x"; then
  AC_MSG_RESULT([auto])
elif test "x$lash_gtk2" = "xyes"; then
  AC_MSG_RESULT([yes])
else
  AC_MSG_RESULT([no])
fi


#####################
### --with-python ###
#####################

lash_python="yes"
AC_MSG_CHECKING([whether to build Python bindings])
AC_ARG_WITH(python,
  [AS_HELP_STRING(--with-python, [build Python bindings [default=yes]])],
  [case "x$withval" in
    "xyes")
      ;;
    "xno")
      lash_python="no"
      ;;
    *)
      AC_MSG_ERROR([must use --with-python(=yes/no) or --without-python])
      ;;
  esac]
)
if test "x$lash_python" = "xyes"; then
  AC_MSG_RESULT([yes])
else
  AC_MSG_RESULT([no])
fi


# Check for all required and selected dependencies. Report all missing
# packages in one go so that the user doesn't need to play hit-and-miss.

MISSING_REQUIRED=""

# 0.99.17 = first version with jack_client_open
HAVE_JACK="no"
PKG_CHECK_MODULES(JACK, jack >= 0.99.17, HAVE_JACK="yes", MISSING_REQUIRED="    JACK >= 0.99.17")

HAVE_DBUS="no"
PKG_CHECK_MODULES(DBUS, dbus-1 >= 1.0.0, HAVE_DBUS="yes", [
  if test "x$MISSING_REQUIRED" != "x"; then
    MISSING_REQUIRED="$MISSING_REQUIRED
    D-Bus >= 1.0.0"
  else
    MISSING_REQUIRED="    D-Bus >= 1.0.0"
  fi
])

HAVE_UUID="no"
PKG_CHECK_MODULES(UUID, uuid, HAVE_UUID="pc",
  [
    AC_CHECK_HEADER(uuid/uuid.h,
      [
        AC_CHECK_LIB(uuid, uuid_generate, HAVE_UUID="lib", )
      ],
    )
    if test "x$HAVE_UUID" = "xno"
    then
      if test "x$MISSING_REQUIRED" != "x"
      then
        MISSING_REQUIRED="$MISSING_REQUIRED
        libuuid (from e2fsprogs)"
      else
        MISSING_REQUIRED="    libuuid (from e2fsprogs)"
      fi
    fi
  ]
)

HAVE_XML2="no"
PKG_CHECK_MODULES(XML2, libxml-2.0 >= 2.0.0, HAVE_XML2="yes", [
  if test "x$MISSING_REQUIRED" != "x"; then
    MISSING_REQUIRED="$MISSING_REQUIRED
    libxml2 >= 2.0.0"
  else
    MISSING_REQUIRED="    libxml2 >= 2.0.0"
  fi
])


MISSING_OPTIONAL=""

HAVE_ALSA=""
if test "x$lash_alsa" != "xno"; then
  PKG_CHECK_MODULES(ALSA, alsa >= 0.9, HAVE_ALSA="maybe", HAVE_ALSA="no")

  if test "x$HAVE_ALSA" = "xmaybe"; then
    # Check that there's seq functions in the library
    ORIG_LIBS="$LIBS"
    LIBS="$LIBS $ALSA_LIBS"
    ORIG_CFLAGS="$CFLAGS"
    CFLAGS="$CFLAGS $ALSA_CFLAGS"

    AC_CHECK_FUNC(snd_seq_open, HAVE_ALSA="yes",
      HAVE_ALSA="no"
      AC_MSG_WARN([The ALSA library you're using doesn't have sequencer functions]))

    LIBS="$ORIG_LIBS"
    CFLAGS="$ORIG_CFLAGS"
  fi

  if test "x$lash_alsa" = "x"; then
    if test "x$HAVE_ALSA" = "xyes"; then
      lash_alsa="yes (auto-selected)"
    else
      lash_alsa="no (auto-selected)"
    fi
  elif test "x$HAVE_ALSA" = "xno"; then
    MISSING_OPTIONAL="    ALSA >= 0.9"
  fi
fi

HAVE_GTK2=""
if test "x$lash_gtk2" != "xno"; then
  PKG_CHECK_MODULES(GTK2, gtk+-2.0 >= 2.0.0, HAVE_GTK2="yes", HAVE_GTK2="no")

  if test "x$lash_gtk2" = "x"; then
    if test "x$HAVE_GTK2" = "xyes"; then
      lash_gtk2="yes (auto-selected)"
    else
      lash_gtk2="no (auto-selected)"
    fi
  elif test "x$HAVE_GTK2" = "xno"; then
    if test "x$MISSING_OPTIONAL" != "x"; then
      MISSING_OPTIONAL="$MISSING_OPTIONAL
    GTK+ >= 2.0.0"
    else
      MISSING_OPTIONAL="    GTK+ >= 2.0.0"
    fi
  fi
fi

HAVE_PYTHON=""
if test "x$lash_python" = "xyes"; then
  AM_PATH_PYTHON([2.3], [HAVE_PYTHON="yes"], [HAVE_PYTHON="no"])
  if test "x$HAVE_PYTHON" = "xyes"; then
    AM_CHECK_PYTHON_HEADERS(, [HAVE_PYTHON="no"])
  fi

  if test "x$HAVE_PYTHON" = "xno"; then
    if test "x$MISSING_OPTIONAL" != "x"; then
      MISSING_OPTIONAL="$MISSING_OPTIONAL
    Python >= 2.3"
    else
      MISSING_OPTIONAL="    Python >= 2.3"
    fi
  fi
fi


if test "x$MISSING_REQUIRED" != "x"; then
    AC_MSG_RESULT([
  The following required dependencies are missing from your system:

$MISSING_REQUIRED])
fi

if test "x$MISSING_OPTIONAL" != "x"; then
    AC_MSG_RESULT([
  The following optionally selected dependencies are missing from your system:

$MISSING_OPTIONAL])
fi

if test "x$MISSING_REQUIRED$MISSING_OPTIONAL" != "x"; then
  AC_MSG_RESULT([
  Please refer to these websites for further information:
])

  if test "x$HAVE_JACK" = "xno"; then
    AC_MSG_RESULT([    JACK:     http://jackaudio.org/])
  fi
  if test "x$HAVE_DBUS" = "xno"; then
    AC_MSG_RESULT([    D-Bus:    http://dbus.freedesktop.org/])
  fi
  if test "x$HAVE_UUID" = "xno"; then
    AC_MSG_RESULT([    libuuid:  http://e2fsprogs.sourceforge.net/])
  fi
  if test "x$HAVE_XML2" = "xno"; then
    AC_MSG_RESULT([    libxml2:  http://xmlsoft.org/])
  fi
  if test "x$HAVE_ALSA" = "xno"; then
    AC_MSG_RESULT([    ALSA:     http://alsa-project.org/])
  fi
  if test "x$HAVE_GTK2" = "xno"; then
    AC_MSG_RESULT([    GTK+ 2:   http://gtk.org/])
  fi
  if test "x$HAVE_PYTHON" = "xno"; then
    AC_MSG_RESULT([    Python:   http://www.python.org/])
  fi

  AC_MSG_RESULT([])

  AC_MSG_ERROR([Missing packages])
fi


###############
### Old API ###
###############

if test "x$lash_old_api" = "xyes"; then
  AC_DEFINE(LASH_OLD_API, 1, [whether or not to enable the old API])
fi
AM_CONDITIONAL(LASH_OLD_API, [test "x$lash_old_api" = "xyes"])

############
### JACK ###
############

JACK_VERSION=$(pkg-config --modversion jack)
AC_DEFINE_UNQUOTED(LASH_JACK_VERSION, "$JACK_VERSION", [The version of jack we're compiling against])
AM_CONDITIONAL(HAVE_JACK_DBUS, [test "x$lash_jack_dbus" = "xyes"])
if test "x$lash_jack_dbus" = "xyes"; then
  AC_DEFINE(HAVE_JACK_DBUS, 1, [Whether to use D-Bus for JACK communication])
else
  AC_SUBST(JACK_CFLAGS)
  AC_SUBST(JACK_LIBS)
  PC_REQUIRES="jack"
fi

#############
### D-Bus ###
#############

DBUS_VERSION=$(pkg-config --modversion dbus-1)
AC_SUBST(DBUS_CFLAGS)
AC_SUBST(DBUS_LIBS)
AC_DEFINE_UNQUOTED(LASH_DBUS_VERSION, "$DBUS_VERSION", [The version of D-Bus we're compiling against])
PC_REQUIRES="$PC_REQUIRES dbus-1"

AC_ARG_ENABLE(pkg-config-dbus-service-dir,
  [  --enable-pkg-config-dbus-service-dir    force D-Bus service install dir to be one returned by pkg-config ],
  DBUS_SERVICES_DIR_FORCE_REAL=$enableval , DBUS_SERVICES_DIR_FORCE_REAL=no ) 

DBUS_SERVICES_DIR_REAL=`$PKG_CONFIG --variable=session_bus_services_dir dbus-1`
if test "x$DBUS_SERVICES_DIR_FORCE_REAL" = "xyes"
then
  AC_MSG_WARN([overriding D-Bus service install dir])
  DBUS_SERVICES_DIR="$DBUS_SERVICES_DIR_REAL"
else
  AS_AC_EXPAND(DATADIR, $datadir)
  DBUS_SERVICES_DIR="$DATADIR/dbus-1/services"
fi
AC_SUBST(DBUS_SERVICES_DIR)
AC_DEFINE_UNQUOTED(DBUS_SERVICES_DIR, "$DBUS_SERVICES_DIR", [Where services dir for DBUS is])

###############
### libuuid ###
###############

if test "x$HAVE_UUID" = "xpc"
then
  UUID_VERSION=$(pkg-config --modversion uuid)
  PC_REQUIRES="$PC_REQUIRES uuid"
else
  UUID_VERSION="unknown"
  UUID_CFLAGS=""
  UUID_LIBS="-luuid"
fi
AC_SUBST(UUID_CFLAGS)
AC_SUBST(UUID_LIBS)
AC_DEFINE_UNQUOTED(LASH_UUID_VERSION, "$UUID_VERSION", [The version of libuuid we're compiling against])


###############
### libxml2 ###
###############

XML2_VERSION=$(pkg-config --modversion libxml-2.0)
AC_SUBST(XML2_CFLAGS)
AC_SUBST(XML2_LIBS)
AC_DEFINE_UNQUOTED(LASH_XML2_VERSION, "$XML2_VERSION", [The version of libxml2 we're compiling against])
PC_REQUIRES="$PC_REQUIRES libxml-2.0"


############
### ALSA ###
############

if test "x$HAVE_ALSA" = "xyes"; then
  ALSA_VERSION=$(pkg-config --modversion alsa)
  AC_SUBST(ALSA_CFLAGS)
  AC_SUBST(ALSA_LIBS)
  AC_DEFINE(HAVE_ALSA, 1, [ALSA sequencer support])
  AC_DEFINE_UNQUOTED(LASH_ALSA_VERSION, "$ALSA_VERSION", [The version of alsa we're compiling against])
  PC_REQUIRES="$PC_REQUIRES alsa"
fi
AM_CONDITIONAL(HAVE_ALSA, [test "x$HAVE_ALSA" = "xyes"])


##############
### GTK+ 2 ###
##############

if test "x$HAVE_GTK2" = "xyes"; then
  GTK2_VERSION=$(pkg-config --modversion gtk+-2.0)
  AC_SUBST(GTK2_CFLAGS)
  AC_SUBST(GTK2_LIBS)
  AC_DEFINE(HAVE_GTK2, 1, [Whether or not GTK+ 2 support is enabled])
  AC_DEFINE_UNQUOTED(LASH_GTK2_VERSION, "$GTK2_VERSION", [The version of GTK+ 2 we're compiling against])
fi
AM_CONDITIONAL(HAVE_GTK2, [test "x$HAVE_GTK2" = "xyes"])


# Check for optionals

################
### Readline ###
################

lash_readline="yes"
VL_LIB_READLINE
AC_SUBST(READLINE_LIBS)

AC_MSG_CHECKING([whether to enable readline])
if test "x$vl_cv_lib_readline" != "xno"; then
  AC_MSG_RESULT([yes])
  AC_DEFINE(HAVE_READLINE, 1, [whether readline library is available])
else
  AC_MSG_RESULT([no])
  lash_readline="no"
fi
AM_CONDITIONAL(HAVE_READLINE, test "x$lash_readline" != "xno")


################
###  Python  ###
################

AM_CONDITIONAL(BUILD_PYLASH, [test "x$HAVE_PYTHON" = "xyes"])


#################
### texi2html ###
#################

AC_CHECK_PROG(lash_texi2html, texi2html, "yes", "no")
if test x$lash_texi2html = xyes; then
  AC_MSG_NOTICE([texi2html found, building manual])
else
  AC_MSG_WARN([texi2html not found, manual will not be built])
fi
AM_CONDITIONAL(HAVE_TEXI2HTML, test x$lash_texi2html = xyes)


##########################
### --with-project-dir ###
##########################

AC_MSG_CHECKING([where to store projects])
AC_ARG_WITH(project-dir,
[AS_HELP_STRING(--with-project-dir=DIR, [store projects in $HOME/DIR [default=audio-projects]])],[
  if test x$withval = xno; then
    AC_MSG_ERROR([use --with-project-dir=DIR, not --without-project-dir])
  fi
  DEFAULT_PROJECT_DIR="$withval";
],[
  DEFAULT_PROJECT_DIR="audio-projects";
])
AC_DEFINE_UNQUOTED(DEFAULT_PROJECT_DIR, "$DEFAULT_PROJECT_DIR", [Where to store projects])
AC_MSG_RESULT($DEFAULT_PROJECT_DIR)


######################
### default CFLAGS ###
######################

LASH_CFLAGS=""
if test "x$lash_debug" = "xyes"; then
  LASH_CFLAGS="-g"
fi
AC_SUBST(LASH_CFLAGS)
AC_MSG_CHECKING(LASH_CFLAGS)
AC_MSG_RESULT($LASH_CFLAGS)

# For the manual
DATE="`date -u`"
AC_SUBST(DATE)

# For the Requires: line in the PC file
AC_SUBST(PC_REQUIRES)
AC_SUBST(PACKAGE_VERSION)

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([lash-1.0.pc])
AC_CONFIG_FILES([m4/Makefile])
AC_CONFIG_FILES([docs/lash-manual.texi])
AC_CONFIG_FILES([docs/Makefile])
AC_CONFIG_FILES([docs/lash-manual-html-one-page/Makefile])
AC_CONFIG_FILES([docs/lash-manual-html-split/Makefile])
AC_CONFIG_FILES([lash/Makefile])
AC_CONFIG_FILES([common/Makefile])
AC_CONFIG_FILES([dbus/Makefile])
AC_CONFIG_FILES([liblash/Makefile])
AC_CONFIG_FILES([lashd/Makefile])
AC_CONFIG_FILES([clients/Makefile])
AC_CONFIG_FILES([clients/simple_client/Makefile])
AC_CONFIG_FILES([clients/shell/Makefile])
AC_CONFIG_FILES([clients/synth/Makefile])
AC_CONFIG_FILES([clients/save_button/Makefile])
AC_CONFIG_FILES([clients/panel/Makefile])
AC_CONFIG_FILES([icons/Makefile])
AC_CONFIG_FILES([pylash/Makefile])


### Print the results ###

AC_OUTPUT

if test "x$lash_readline" = "xno"; then
  lash_readline="no (lash_shell will not be built)"
fi

AC_MSG_RESULT([
  LASH Configured

  JACK D-Bus support:    $lash_jack_dbus
  ALSA MIDI support:     $lash_alsa
  GTK+ 2 clients:        $lash_gtk2
  Readline support:      $lash_readline
  Python bindings:       $lash_python
  HTML manual:           $lash_texi2html
  Debugging output:      $lash_debug
  Support old API:       $lash_old_api

  JACK version:          $JACK_VERSION
  D-Bus version:         $DBUS_VERSION
  libuuid version:       $UUID_VERSION
  libxml2 version:       $XML2_VERSION])

if test x$HAVE_ALSA = xyes; then
  AC_MSG_RESULT([  ALSA version:          $ALSA_VERSION])
fi

if test x$HAVE_GTK2 = xyes; then
  AC_MSG_RESULT([  GTK+ 2 version:        $GTK2_VERSION])
fi

AC_MSG_RESULT([
  CFLAGS:  $CFLAGS

  JACK_CFLAGS:  $JACK_CFLAGS
  DBUS_CFLAGS:  $DBUS_CFLAGS
  UUID_CFLAGS:  $UUID_CFLAGS
  XML2_CFLAGS:  $XML2_CFLAGS])

if test x$HAVE_ALSA = xyes; then
  AC_MSG_RESULT([  ALSA_CFLAGS:  $ALSA_CFLAGS])
fi

if test x$HAVE_GTK2 = xyes; then
  AC_MSG_RESULT([  GTK2_CFLAGS:  $GTK2_CFLAGS])
fi

AC_MSG_RESULT([
  JACK_LIBS:    $JACK_LIBS
  DBUS_LIBS:    $DBUS_LIBS
  UUID_LIBS:    $UUID_LIBS
  XML2_LIBS:    $XML2_LIBS])

if test x$HAVE_ALSA = xyes; then
  AC_MSG_RESULT([  ALSA_LIBS:    $ALSA_LIBS])
fi

if test x$HAVE_GTK2 = xyes; then
  AC_MSG_RESULT([  GTK2_LIBS:    $GTK2_LIBS])
fi

if test x$lash_readline = xyes; then
  AC_MSG_RESULT([  READLINE_LIBS: $READLINE_LIBS])
fi

AC_MSG_RESULT([
  Default project store:     $DEFAULT_PROJECT_DIR
  Installation prefix:       $prefix
  D-Bus service install dir: $DBUS_SERVICES_DIR])

if test "x$DBUS_SERVICES_DIR_REAL" != "x$DBUS_SERVICES_DIR"
then
	AC_MSG_WARN([D-Bus session services directory as reported by pkg-config is $DBUS_SERVICES_DIR_REAL])
	AC_MSG_WARN([but service file will be installed in $DBUS_SERVICES_DIR])
	AC_MSG_WARN([You may need to adjust your D-Bus configuration after installing jackdbus])
	AC_MSG_WARN([You can override dbus service install dir])
	AC_MSG_WARN([with --enable-pkg-config-dbus-service-dir option to this script])
fi
