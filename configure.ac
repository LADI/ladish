AC_INIT([LASH],[0.5.0])
AC_CONFIG_SRCDIR([lash/types.h])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

### Check for programs ###
AC_LANG([C])
AC_PROG_CC
AC_PROG_LIBTOOL

#################
### texi2html ###
#################
AC_CHECK_PROG(lash_texi2html, texi2html, "yes", "no")
AM_CONDITIONAL(HAVE_TEXI2HTML, test x$lash_texi2html = xyes)

############
### JACK ###
############
PKG_CHECK_MODULES(JACK, jack >= 0.50.0, true, AC_MSG_ERROR([you need jack >= 0.50.0.  see http://sf.net/projects/jackit]))
JACK_VERSION=$( pkg-config --modversion jack )
AC_SUBST(JACK_CFLAGS)
AC_SUBST(JACK_LIBS)
AC_DEFINE_UNQUOTED(JACK_VERSION, "$JACK_VERSION", [The version of jack we're compiling against])


############
### ALSA ###
############
PKG_CHECK_MODULES(ALSA, alsa >= 0.9, true, AC_MSG_ERROR([you need alsa >= 0.9. see http://www.alsa-project.org/]))

# check that there's seq functions in the library
ORIG_LIBS="$LIBS"
LIBS="$LIBS $ALSA_LIBS"
ORIG_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS $ALSA_CFLAGS"

AC_CHECK_FUNC(snd_seq_open, true,
  AC_MSG_ERROR([the alsa library you're using doesn't have sequencer functions]))

LIBS="$ORIG_LIBS"
CFLAGS="$ORIG_CFLAGS"

ALSA_VERSION=$( pkg-config --modversion alsa )
AC_SUBST(ALSA_CFLAGS)
AC_SUBST(ALSA_LIBS)
AC_DEFINE_UNQUOTED(ALSA_VERSION, "$ALSA_VERSION", [The version of alsa we're compiling against])

###############
### libxml2 ###
###############
PKG_CHECK_MODULES(XML2, libxml-2.0 >= 2.0.0, true, AC_MSG_ERROR([you need libxml2 >= 2.0.0.  see http://xmlsoft.org/]))
JACK_VERSION=$( pkg-config --modversion libxml-2.0 )
AC_SUBST(XML2_CFLAGS)
AC_SUBST(XML2_LIBS)
AC_DEFINE_UNQUOTED(XML2_VERSION, "$XML2_VERSION", [The version of libxml2 we're compiling against])


############
### uuid ###
############
AC_CHECK_HEADER(uuid/uuid.h, UUID_FOUND="yes", UUID_FOUND="no")
if test x$UUID_FOUND = xno; then
  AC_MSG_ERROR([libuuid, from the e2fsprogs prackage, is required for lashd.  see http://e2fsprogs.sourceforge.net/])
fi

AC_CHECK_LIB(uuid, uuid_generate, UUID_FOUND="yes", UUID_FOUND="no")
if test x$UUID_FOUND = xno; then
  AC_MSG_ERROR([libuuid, from the e2fsprogs prackage, is required for lashd. see http://e2fsprogs.sourceforge.net/])
fi
UUID_LIBS="-luuid"
AC_SUBST(UUID_CFLAGS)
AC_SUBST(UUID_LIBS)


##############
### GTK+ 2 ###
##############
lash_gtk2="yes"
AC_ARG_ENABLE(gtk2,
[  --disable-gtk2          disable the gtk 2 test client],[
  case "$enableval" in
    "yes")
      ;;
    "no")
      lash_gtk2="no";
      ;;
    *)
      AC_MSG_ERROR([must use --enable-gtk2(=yes/no) or --disable-gtk2])
      ;;
  esac
])

if test x$lash_gtk2 = xyes; then
  PKG_CHECK_MODULES(GTK2, gtk+-2.0 >= 2.0.0, GTK2_FOUND="yes", GTK2_FOUND="no")
  AC_SUBST(GTK2_CFLAGS)
  AC_SUBST(GTK2_LIBS)

  if test x$GTK2_FOUND = xyes; then
    AC_DEFINE(HAVE_GTK2, 1, "whether or not we have gtk 2 support")
  fi
else
  GTK2_FOUND="no"
fi
lash_gtk2=$GTK2_FOUND
AM_CONDITIONAL(HAVE_GTK2, test x$GTK2_FOUND = xyes)




################
### readline ###
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


##############
### GTK+ 1 ###
##############
lash_gtk="yes"
AC_ARG_ENABLE(gtk,
[  --disable-gtk           disable the gtk test client],[
  case "$enableval" in
    "yes")
      ;;
    "no")
      lash_gtk="no";
      ;;
    *)
      AC_MSG_ERROR([must use --enable-gtk(=yes/no) or --disable-gtk])
      ;;
  esac
])

if test x$lash_gtk = xyes; then
  AM_PATH_GTK(1.2.0, GTK_FOUND="yes", GTK_FOUND="no")
  lash_gtk=$GTK_FOUND
fi

AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)
AM_CONDITIONAL(HAVE_GTK, test x$GTK_FOUND = xyes)


######################
### --enable-debug ###
######################
AC_MSG_CHECKING([whether to enable debugging output])
lash_debug="no"
AC_ARG_ENABLE(debug,
[  --enable-debug          enable debugging output (from everything)],[
  case "$enableval" in
    "yes")
      lash_debug="yes"
      AC_DEFINE(LASH_DEBUG, 1, [whether or not to enable debugging output])
      ;;
    "no")
      ;;
    *)
      AC_MSG_ERROR([must use --enable-debug(=yes/no) or --disable-debug])
      ;;
  esac
])

if test x$lash_debug = xyes; then
  AC_MSG_RESULT([yes])
else
  AC_MSG_RESULT([no])
fi

###########################
### --disable-serv-inst ###
###########################
AC_MSG_CHECKING([whether to install service in /etc/services])
lash_serv_inst="yes"
AC_ARG_ENABLE(serv-inst,
[  --disable-serv-inst     disable installation of lash service in /etc/services],[
  case "$enableval" in
    "yes")
      ;;
    "no")
      lash_serv_inst="no"
      ;;
    *)
      AC_MSG_ERROR([must use --enable-serv-inst(=yes/no) or --disable-serv-inst])
      ;;
  esac
])

AM_CONDITIONAL(SERVICE_INSTALL, test x$lash_serv_inst = xyes)
if test x$lash_serv_inst = xyes; then
  AC_MSG_RESULT([yes])
else
  AC_MSG_RESULT([no])
fi

##########################
### --with-default-dir ###
##########################
AC_MSG_CHECKING([where to create projects])
AC_ARG_WITH(default-dir,
[  --with-default-dir=DIR  tell the server to use \$HOME/DIR to store project directories in],[
  if test x$withval = xno; then
    AC_MSG_ERROR([use --with-default-dir=DIR, not --without-default-dir])
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
LASH_CFLAGS="-DLASH_BUILD -DHAVE_LASH"
LASH_LIBS=""
if test x$lash_debug = xyes; then
  LASH_CFLAGS="$LASH_CFLAGS -g"
  LASH_LIBS=""
#  LASH_LIBS="/usr/lib/gcrt1.o -lc_p"
fi
AC_SUBST(LASH_CFLAGS)
AC_SUBST(LASH_LIBS)
AC_MSG_CHECKING(LASH_CFLAGS)
AC_MSG_RESULT($LASH_CFLAGS)
AC_MSG_CHECKING(LASH_LIBS)
AC_MSG_RESULT($LASH_LIBS)


AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([lash-1.0.pc])
AC_CONFIG_FILES([m4/Makefile])
AC_CONFIG_FILES([docs/Makefile])
AC_CONFIG_FILES([docs/lash-manual-html-one-page/Makefile])
AC_CONFIG_FILES([docs/lash-manual-html-split/Makefile])
AC_CONFIG_FILES([lash/Makefile])
AC_CONFIG_FILES([liblash/Makefile])
AC_CONFIG_FILES([lashd/Makefile])
AC_CONFIG_FILES([clients/Makefile])
AC_CONFIG_FILES([clients/simple_client/Makefile])
AC_CONFIG_FILES([clients/control/Makefile])
AC_CONFIG_FILES([clients/gtk_client/Makefile])
AC_CONFIG_FILES([clients/synth/Makefile])
AC_CONFIG_FILES([clients/save_button/Makefile])
AC_CONFIG_FILES([clients/panel/Makefile])
AC_CONFIG_FILES([icons/Makefile])

AC_OUTPUT


if test "$lash_readline" = "no"; then
  lash_readline="no (lash-control will not be built)"
fi

if test "$lash_gtk2" = "no"; then
  lash_gtk2="no (gtk 2 clients will not be built)"
fi

if test "$lash_gtk" = "no"; then
  lash_gtk="no (gtk 1 clients will not be built)"
fi

if test "$lash_texi2html" = "no"; then
  lash_texi2html="no (html manual will not be built)"
fi

AC_MSG_RESULT([

  LASH configured

  readline support:        $lash_readline
  gtk 2 support:           $lash_gtk2
  gtk 1 support:           $lash_gtk
  debugging output:        $lash_debug
  texi2html:               $lash_texi2html

  JACK_CFLAGS:   $JACK_CFLAGS
  ALSA_CFLAGS:   $ALSA_CFLAGS
  XML2_CFLAGS:   $XML2_CFLAGS ])

if test x$GTK2_FOUND = xyes; then
  AC_MSG_RESULT([  GTK2_CFLAGS:   $GTK2_CFLAGS ])
fi
if test x$GTK_FOUND = xyes; then
  AC_MSG_RESULT([  GTK_CFLAGS:    $GTK_CFLAGS ])
fi

AC_MSG_RESULT([  JACK_LIBS:     $JACK_LIBS ])
AC_MSG_RESULT([  ALSA_LIBS:     $ALSA_LIBS ])
AC_MSG_RESULT([  XML2_LIBS:     $XML2_LIBS ])

if test x$GTK2_FOUND = xyes; then
  AC_MSG_RESULT([  GTK2_LIBS:     $GTK2_LIBS ])
fi
if test x$GTK_FOUND = xyes; then
  AC_MSG_RESULT([  GTK_LIBS:      $GTK_LIBS ])
fi
if test x$lash_readline = xyes; then
  AC_MSG_RESULT([  READLINE_LIBS: $READLINE_LIBS])
fi

AC_MSG_RESULT([

  Default project store:   $DEFAULT_PROJECT_DIR
  Installation prefix:     $prefix


])

