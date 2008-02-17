AC_PREREQ(2.59)
AC_INIT([drobillad],[svn],[dave@drobilla.net])

m4_include([m4/ac_python_devel.m4])
AC_CONFIG_AUX_DIR([config])


################### SOURCES

# LV2 Core
AC_CONFIG_SRCDIR([lv2core/lv2.h])

# SLV2
AC_CONFIG_SRCDIR([slv2/src/plugin.c])
AC_CONFIG_SRCDIR([slv2/slv2/plugin.h])
AC_CONFIG_SRCDIR([slv2/utils/lv2_list.c])
AC_CONFIG_SRCDIR([slv2/swig/slv2.i])

# See slv2/configure.ac for versioning info/history
SLV2_VERSION="0.6.0"
SLV2_API_CURRENT="9"
SLV2_API_REVISION="0"
SLV2_API_AGE="0"

AC_SUBST(SLV2_API_CURRENT)
AC_SUBST(SLV2_API_REVISION)
AC_SUBST(SLV2_API_AGE)
SLV2_SO_VERSION="${SLV2_API_CURRENT}:${SLV2_API_REVISION}:${SLV2_API_AGE}"
AC_SUBST(SLV2_SO_VERSION)
AC_SUBST(SLV2_VERSION)

# Redlandmm
AC_CONFIG_SRCDIR([redlandmm/redlandmm/Model.h])
AC_CONFIG_SRCDIR([redlandmm/src/Model.cpp])

# Raul
AC_CONFIG_SRCDIR([raul/raul/Semaphore.h])

# See raul/configure.ac for versioning info/history
RAUL_VERSION="0.4.0"
RAUL_API_CURRENT="0"
RAUL_API_REVISION="0"
RAUL_API_AGE="0"

AC_SUBST(RAUL_API_CURRENT)
AC_SUBST(RAUL_API_REVISION)
AC_SUBST(RAUL_API_AGE)
RAUL_SO_VERSION="${RAUL_API_CURRENT}:${RAUL_API_REVISION}:${RAUL_API_AGE}"
AC_SUBST(RAUL_SO_VERSION)
AC_SUBST(RAUL_VERSION)

# FlowCanvas
AC_CONFIG_SRCDIR([flowcanvas/src/Canvas.cpp])
AC_CONFIG_SRCDIR([flowcanvas/flowcanvas/Canvas.hpp])

# See flowcanvas/configure.ac for versioning info/history
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

# Patchage
AC_CONFIG_SRCDIR([src/main.cpp])

# Omins
AC_CONFIG_SRCDIR([omins/src/hz_voct_4200.c])

# Ingen
AC_CONFIG_SRCDIR([ingen/src/common/interface/EngineInterface.hpp])
AC_CONFIG_SRCDIR([ingen/src/common/lv2ext/lv2-miditype.h])
AC_CONFIG_SRCDIR([ingen/src/libs/module/Module.cpp])
AC_CONFIG_SRCDIR([ingen/src/libs/engine/JackAudioDriver.cpp])
AC_CONFIG_SRCDIR([ingen/src/libs/serialisation/Loader.cpp])
AC_CONFIG_SRCDIR([ingen/src/libs/client/OSCController.cpp])
AC_CONFIG_SRCDIR([ingen/src/libs/gui/App.cpp])
AC_CONFIG_SRCDIR([ingen/src/progs/ingen/main.cpp])

# Machina
AC_CONFIG_SRCDIR([machina/src/main.cpp])
AC_CONFIG_SRCDIR([machina/src/engine/Machine.cpp])
AC_CONFIG_SRCDIR([machina/src/engine/machina/Machine.hpp])
AC_CONFIG_SRCDIR([machina/src/gui/main.cpp])

# LV2 stuff
AC_CONFIG_SRCDIR([lv2/lv2/osc/lv2_osc.h])

# LV2 plugin install
AC_MSG_CHECKING([where to install LV2 plugins])
AC_ARG_WITH(lv2-dir,
	AS_HELP_STRING([--with-lv2-dir=DIR],
		[directory where LV2 plugins should be installed ($libdir/lv2)]),
	[lv2plugindir=$withval], [lv2plugindir=$libdir/lv2])
AC_MSG_RESULT($lv2plugindir)
AC_SUBST(lv2plugindir)

AC_CONFIG_HEADER([config/config.h])

AM_INIT_AUTOMAKE

#################### ENVIRONMENT

AC_LANG([C++])

AC_PROG_CXX
AM_PROG_CC_C_O

AC_DISABLE_STATIC
AC_PROG_LIBTOOL

PKG_CHECK_MODULES(GLIBMM, glibmm-2.4)
PKG_CHECK_MODULES(GTHREAD, gthread-2.0)

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

REDLANDMM_CFLAGS="-I$abs_srcdir/redlandmm"
REDLANDMM_LIBS="$builddir/redlandmm/src/.libs/libredlandmm.la"
REDLANDMM_SRCDIR="$abs_srcdir/redlandmm"
AC_SUBST(REDLANDMM_CFLAGS)
AC_SUBST(REDLANDMM_LIBS)
AC_SUBST(REDLANDMM_SRCDIR)

RAUL_CFLAGS="-I$abs_srcdir/raul"
RAUL_LIBS="$builddir/raul/src/.libs/libraul.la"
RAUL_SRCDIR="$abs_srcdir/raul"
AC_SUBST(RAUL_CFLAGS)
AC_SUBST(RAUL_LIBS)
AC_SUBST(RAUL_SRCDIR)

FLOWCANVAS_CFLAGS="-I$abs_srcdir/flowcanvas"
FLOWCANVAS_LIBS="$builddir/flowcanvas/src/.libs/libflowcanvas.la"
FLOWCANVAS_SRCDIR="$abs_srcdir/flowcanvas"
AC_SUBST(FLOWCANVAS_CFLAGS)
AC_SUBST(FLOWCANVAS_LIBS)
AC_SUBST(FLOWCANVAS_SRCDIR)

LV2CORE_VERSION="1"
LV2CORE_CFLAGS="-I$abs_srcdir/lv2core"
AC_SUBST(LV2CORE_VERSION)
AC_SUBST(LV2CORE_CFLAGS)

DROBINS_CFLAGS="$LV2CORE_CFLAGS -I$abs_srcdir/lv2"
DROBINS_SRCDIR="$abs_srcdir/lv2"
AC_SUBST(DROBINS_CFLAGS)
AC_SUBST(DROBINS_SRCDIR)

LV2_OSC_CFLAGS="$LV2CORE_CFLAGS -I$abs_srcdir/lv2 -I$abs_srcdir/lv2/lv2/event"
LV2_OSC_LIBS="$builddir/lv2/lv2/osc/.libs/liblv2_osc.la"
AC_SUBST(LV2_OSC_CFLAGS)
AC_SUBST(LV2_OSC_LIBS)

SLV2_CFLAGS="$LV2CORE_CFLAGS -I$abs_srcdir/slv2"
SLV2_LIBS="$builddir/slv2/src/.libs/libslv2.la"
SLV2_SRCDIR="$abs_srcdir/slv2"
AC_SUBST(SLV2_CFLAGS)
AC_SUBST(SLV2_LIBS)
AC_SUBST(SLV2_SRCDIR)
	
INGEN_CFLAGS="-I$abs_srcdir/ingen/src/common -I$abs_srcdir/ingen/src/libs -I$abs_srcdir/ingen/src/libs/engine/events -I$abs_srcdir/ingen/src"
AC_SUBST(INGEN_CFLAGS)

CONFIG_H_PATH="$builddir/config/config.h"

#################### COMMAND LINE PARAMETERS

eugene="no"
AC_ARG_ENABLE(eugene,
	[AS_HELP_STRING(--enable-eugene, [Use Eugene for machina evolution (no)])]
	[eugene="$enableval"])
if test "$eugene" = "yes"; then
	PKG_CHECK_MODULES(EUGENE, eugene-core)
	AC_DEFINE(HAVE_EUGENE, 1, [Has Eugene])
fi
AM_CONDITIONAL(WITH_EUGENE, [test "$eugene" = "yes"])

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
	[assertions="$enableval"])

if test "$debug_symbols" = "yes"; then
  CFLAGS="-O0 -g"
  CXXFLAGS="-O0 -g"
fi

if test "$debug_assertions" = "yes"; then
  CFLAGS="$CFLAGS -DDEBUG -DLIBRDF_DEBUG"
  CXXFLAGS="$CXXFLAGS -DDEBUG -DLIBRDF_DEBUG"
else
  CFLAGS="$CFLAGS -DNDEBUG -DLIBRDF_DEBUG"
  CXXFLAGS="$CXXFLAGS -DNDEBUG -DLIBRDF_DEBUG"
fi

# Boost shared_ptr debugging
pointer_debug="no"
AC_ARG_ENABLE(pointer_debug,
	[AS_HELP_STRING(--enable-pointer-debug, [Smart pointer debugging (DEVELOPERS ONLY) (no)])],
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

# LV2 core package
#PKG_CHECK_MODULES(LV2CORE, lv2core >= 1)

# Jack support
build_jack="yes"
AC_ARG_ENABLE(enable-jack,
	[AS_HELP_STRING(--enable-jack, [Enable Jack support (yes)])],
	[ if test x$enable_jack = xno ; then build_jack=no ; fi ])
if test "$build_jack" = "yes"; then
	PKG_CHECK_MODULES(JACK, jack >= 0.107.0)
	AC_DEFINE(HAVE_JACK, 1, [Has Jack])
	AC_DEFINE(HAVE_JACK_MIDI, 1, [Has Jack MIDI])
fi

# LV2 support
build_lv2="yes"
AC_ARG_ENABLE(lv2,
	[AS_HELP_STRING(--enable-lv2, [Enable LV2 plugin support through libslv2 (yes) - Requires: libslv2])],
	[ if test x$enable_lv2 = xno ; then build_lv2=no ; fi ])
if test "$build_lv2" = "yes"; then
	# Configure SLV2 (independent build system)
#	AC_CONFIG_SUBDIRS([slv2])
	AC_DEFINE(HAVE_SLV2, 1, [Has libslv2])
fi
	
# Build LV2 GTK GUI
build_lv2_gui="yes"
AC_ARG_ENABLE(lv2-gui,
	[AS_HELP_STRING(--enable-lv2-gui, [Build plugin GUIs (requires Gtk2)])],
	[ if test x$enable_lv2_gui = xno ; then build_lv2_gui=no ; fi ])
if test "$build_lv2_gui" = "yes"; then
	PKG_CHECK_MODULES(GTK, gtk+-2.0)
fi
AM_CONDITIONAL(WITH_LV2_GUI, [test "$build_lv2_gui" = "yes"])


# LADSPA support
build_ladspa="yes"
AC_ARG_ENABLE(ladspa,
	[AS_HELP_STRING(--enable-ladspa, [Enable LADSPA plugin support (yes)])],
	[ if test x$enable_ladspa = xno ; then build_ladspa=no ; fi ])
if test "$build_ladspa" = "yes"; then
	build_ladspa="no"
	AC_CHECK_HEADER([ladspa.h], [build_ladspa="yes"],
		[AC_MSG_WARN([You don't seem to build ladspa.h, Ingen will not be very useful!])])
fi
if test "$build_ladspa" = "yes"; then
	AC_DEFINE(HAVE_LADSPA, 1, [Has ladspa.h])
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

# SWIG bindings
bindings="no"
AC_ARG_ENABLE(bindings,
	[AS_HELP_STRING(--enable-bindings, [Build language bindings via SWIG (false)])],
	[bindings="$enableval"])
if test "$bindings" = "yes"; then
  AC_CHECK_PROGS(SWIG, "swig swig-1.3")
  if test "$SWIG" = ""; then
  	AC_MSG_WARN("SWIG not found, language bindings disabled")
  fi
fi
AM_CONDITIONAL(WITH_SWIG, [test "$bindings" = "yes"])

# Check for binding languages
mzscheme="no"
chicken="no"
if test "$bindings" = "yes"; then
  AC_PYTHON_DEVEL([>= '2.3.0'])
  AC_CHECK_HEADER([plt/escheme.h], [mzscheme="yes"], [mzscheme="no"])
  AC_CHECK_HEADER([chicken.h], [chicken="yes"], [chicken="no"])
fi
AM_CONDITIONAL(WITH_PYTHON, [test ! "$PYTHON_VERSION" = "yes"])
AM_CONDITIONAL(WITH_MZSCHEME, [test "$mzscheme" = "yes"])
AM_CONDITIONAL(WITH_CHICKEN, [test "$chicken" = "yes"])

# Build unit tests
build_unit_tests="no"
AC_ARG_ENABLE(unit-tests,
	[AS_HELP_STRING(--enable-unit-tests, [Build unit tests (no) - Developers only])],
	[build_unit_tests="$enableval"])

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

AM_CONDITIONAL(WITH_ALSA, [test "$ALSA_FOUND" = "yes"])
AM_CONDITIONAL(WITH_LASH, [test "$have_lash" = "yes"])
AM_CONDITIONAL(WITH_JACK, [test "$build_jack" = "yes"])
AM_CONDITIONAL(BUILD_SERVER, [test "$build_ingen_server" = "yes"])
AM_CONDITIONAL(BUILD_IN_PROCESS_ENGINE, [test "$build_in_process_engine" = "yes"])
AM_CONDITIONAL(BUILD_UNIT_TESTS, [test "$build_unit_tests" = "yes"])
AM_CONDITIONAL(WITH_LADSPA, [test "$build_ladspa" = "yes"])
AM_CONDITIONAL(WITH_LV2, [test "$build_lv2" = "yes"])


################# INGEN OPTIONS

# Build support for running as an in-process jack client
build_in_process_engine="no"
AC_ARG_ENABLE([in-process-engine],
	AS_HELP_STRING(--enable-in-process-engine, Build engine as an in-process Jack client (no)),
	[ if test x$enable_in_process_engine = xyes ; then build_in_process_engine=yes ; fi ])

if test "$build_in_process_engine" = "yes"; then
	AC_DEFINE(BUILD_IN_PROCESS_ENGINE, 1, [Whether to support running as a Jack in-process client])
fi

# Stand-alone engine (OSC controlled)
build_ingen_server="yes"
AC_ARG_ENABLE([ingen-server],
	AS_HELP_STRING(--enable-server, Build OSC controlled stand-alone engine (yes)),
	[ if test x$enable_server = xno ; then build_ingen_server=no ; fi ])

# Command-line clients
build_ingen_client_lib="yes"
AC_ARG_ENABLE([ingen-clients],
	AS_HELP_STRING(--enable-ingen-clients, [Build Ingen client library (yes) - Requires: libxml2, raptor, libsigc++]),
	[ if test x$enable_ingen_clients = xno ; then build_ingen_client_lib=no ; fi ])

# Gtk client (Ingenuity)
build_ingen_gtk_client="yes"
AC_ARG_ENABLE([ingen-gtk-client],
	AS_HELP_STRING(--enable-ingen-gtk-client, [Build Ingen GTK client (true)]),
	[ if test x$enable_gtk_client = xno ; then build_gtk_client=no ; fi ])

# OSC via Liblo
build_liblo="yes"
PKG_CHECK_MODULES(LIBLO, liblo, [build_liblo="yes"], [build_liblo="no"])
if test "$build_liblo" = "yes"; then
	AC_DEFINE(HAVE_LIBLO, 1, [Has the liblo library])
fi

if test "$build_ingen_gtk_client" = "yes"; then
	build_ingen_clients="yes"
fi

# Clients
if test "$build_ingen_client_lib" = "yes"; then
	AC_CHECK_HEADERS([sys/time.h unistd.h])

	# Check for libxml2 # FIXME: deprecated, make optional
	PKG_CHECK_MODULES(LXML2, libxml-2.0 >= 2.6.0)
	
	# Check for raptor (for RDF serialization)
	PKG_CHECK_MODULES(RAPTOR, raptor >= 1.4.14, build_raptor="yes", build_raptor="no")
	
	# Check for rasqal (for RDF querying)
	PKG_CHECK_MODULES(RASQAL, rasqal >= 0.9.14, build_rasqal="yes", build_rasqal="no")
	
	# Check for sigc++ (FIXME: make this only necessary where.. uh.. necessary)
	PKG_CHECK_MODULES(LSIGCPP, sigc++-2.0)
else
 	AC_MSG_WARN([Ingen client library/executable will not be built!])
fi
AM_CONDITIONAL(BUILD_INGEN_CLIENTS, [test "$build_ingen_clients" = "yes"])

if test "$build_ingen_gtk_client" = "yes"; then
#	AC_CHECK_HEADERS([sys/time.h unistd.h])
#	AC_CHECK_HEADERS([string.h sys/time.h unistd.h])
#	AC_HEADER_TIME
#	AC_FUNC_ERROR_AT_LINE
#	AC_FUNC_FORK
#	AC_FUNC_MALLOC
#	AC_FUNC_STAT
#	AC_CHECK_FUNCS([gettimeofday mkdir strcasecmp strchr strdup strtol])

	PKG_CHECK_MODULES(GTKMM, gtkmm-2.4)
	PKG_CHECK_MODULES(GNOMECANVASMM, libgnomecanvasmm-2.6)
	PKG_CHECK_MODULES(LIBGLADEMM, libglademm-2.4)
	#PKG_CHECK_MODULES(FLOWCANVAS, flowcanvas >= 0.1.0)
	PKG_CHECK_MODULES(CURL, libcurl >= 7.15.0, build_curl="yes", build_curl="no")
	if test "$build_curl" = "yes"; then
		AC_DEFINE(HAVE_CURL, 1, [Has the libcurl library])
	fi
else
  AC_MSG_WARN([GTK client will NOT be built!])
fi
AM_CONDITIONAL(BUILD_GTK_CLIENT, [test "$build_ingen_gtk_client" = "yes"])
AM_CONDITIONAL(WITH_CURL, [test "$build_curl" = "yes"])

# Build ingen client lib if anything above that depends on it is to be built
AM_CONDITIONAL(BUILD_CLIENT_LIB, [test "$build_ingen_clients" = "yes" -o "$build_ingen_gtk_client" = "yes"])


############# PATCHAGE OPTIONS
PKG_CHECK_MODULES(GTKMM, gtkmm-2.4)
PKG_CHECK_MODULES(GNOMECANVASMM, libgnomecanvasmm-2.6)
PKG_CHECK_MODULES(LIBGLADEMM, libglademm-2.4)

################ SLV2 / Redlandmm

# Check for Redland
build_redland="no"
AC_CHECK_PROG(REDLAND_CONFIG, redland-config, redland-config)
if test "X$REDLAND_CONFIG" = X; then
  AC_MSG_ERROR([This software requires Redland (librdf), but redland-config not found.])
else
  build_redland="yes"
  REDLAND_CFLAGS=`$REDLAND_CONFIG --cflags`
  REDLAND_LIBS=`$REDLAND_CONFIG --libs`
  AC_SUBST(REDLAND_CFLAGS)
  AC_SUBST(REDLAND_LIBS)
fi
AM_CONDITIONAL(WITH_REDLAND, [test "$build_redland" = "yes"])


################## RAUL

# (No required dependencies)

########### FLOWCANVAS

# Check for anti-aliased flag
anti_alias="yes"
AC_ARG_ENABLE(anti-aliasing,
	[AS_HELP_STRING(--enable-anti-aliasing, [Enable anti aliased canvas (yes)])],
	[ anti_alias="$enableval"])
# Yes, this is dirty...
if test "$anti_alias" = "yes"; then
	AC_CONFIG_COMMANDS([set_aa],
		[sed 's/\(\/\*CANVASBASE\*\/\).*/\1Gnome::Canvas::CanvasAA/' -i $srcdir/flowcanvas/flowcanvas/Canvas.hpp])
else
	AC_CONFIG_COMMANDS([set_no_aa],
		[sed 's/\(\/\*CANVASBASE\*\/\).*/\1Gnome::Canvas::Canvas/' -i $srcdir/flowcanvas/flowcanvas/Canvas.hpp])
fi

# Check for the graphviz library for auto-layout support
PKG_CHECK_MODULES(AGRAPH, libgvc >= 2.8, graphviz="yes", graphviz="no")
if test "$graphviz" = "yes"; then
	AC_DEFINE(HAVE_AGRAPH, 1, [Has the graphviz library])
else
	AC_MSG_WARN([Warning: Graphviz 'agraph' library (graphviz-dev) not found, no auto-arrange support
				 will be built])
fi


############### MACHINA

# Build Machina GUI
build_machina_gui="yes"
AC_ARG_ENABLE(machina-gui,
	[AS_HELP_STRING(--enable-machina-gui, [Build Machina GUI (yes)])],
	[build_machina_gui="$enableval"])
if test "$build_machina_gui" = "yes"; then
	PKG_CHECK_MODULES(GTKMM, gtkmm-2.4)
	PKG_CHECK_MODULES(GNOMECANVASMM, libgnomecanvasmm-2.6)
	PKG_CHECK_MODULES(LIBGLADEMM, libglademm-2.4)
	#PKG_CHECK_MODULES(FLOWCANVAS, flowcanvas >= 0.1.0)
fi
AM_CONDITIONAL(WITH_MACHINA_GUI, [test "x$build_machina_gui" = "xyes"])


####################### OMINS

# Check plugin install directory
AC_MSG_CHECKING([where to install LADSPA plugins])
AC_ARG_WITH(ladspa-dir,
	AS_HELP_STRING([--with-ladspa-dir=DIR],
		[directory where LADSPA plugins should be installed ($libdir/ladspa)]),
	[ladspadir=$withval], [ladspadir=$libdir/ladspa])
AC_MSG_RESULT($ladspadir)
AC_SUBST(ladspadir)

# Doxygen is required to make dist
AC_CHECK_PROG(HAVE_DOXYGEN, doxygen, true, false)
if test $HAVE_DOXYGEN = "false"; then
	AC_MSG_WARN([Doxygen not found, documentation will not be built])
fi
AM_CONDITIONAL(HAVE_DOXYGEN, $HAVE_DOXYGEN)


#################### OUTPUT

# Omins
AC_CONFIG_FILES([omins/Makefile])
AC_CONFIG_FILES([omins/src/Makefile])

# LV2 Core
AC_CONFIG_FILES([lv2core/Makefile])
AC_CONFIG_FILES([lv2core/lv2core.pc])
AC_CONFIG_FILES([lv2core/lv2core.lv2/Makefile])

# SLV2
AC_CONFIG_FILES([slv2/Makefile])
AC_CONFIG_FILES([slv2/src/Makefile])
AC_CONFIG_FILES([slv2/slv2/Makefile])
AC_CONFIG_FILES([slv2/utils/Makefile])
AC_CONFIG_FILES([slv2/hosts/Makefile])
AC_CONFIG_FILES([slv2/swig/Makefile])
AC_CONFIG_FILES([slv2/slv2.pc])
AC_CONFIG_FILES([slv2/doc/Makefile])
AC_CONFIG_FILES([slv2/doc/reference.doxygen])
AC_CONFIG_FILES([slv2/debian/Makefile])

# LV2 Stuff
AC_CONFIG_FILES([lv2/Makefile])
AC_CONFIG_FILES([lv2/lv2/Makefile])
AC_CONFIG_FILES([lv2/lv2/event/Makefile])
AC_CONFIG_FILES([lv2/lv2/event/lv2_event.pc])
AC_CONFIG_FILES([lv2/lv2/midi/Makefile])
AC_CONFIG_FILES([lv2/lv2/osc/Makefile])
AC_CONFIG_FILES([lv2/lv2/osc/lv2_osc.pc])
AC_CONFIG_FILES([lv2/lv2/contexts/Makefile])
AC_CONFIG_FILES([lv2/lv2/variables/Makefile])
AC_CONFIG_FILES([lv2/lv2/ui/Makefile])
AC_CONFIG_FILES([lv2/lv2/object/Makefile])
AC_CONFIG_FILES([lv2/lv2/uri_map/Makefile])
AC_CONFIG_FILES([lv2/plugins/Makefile])
AC_CONFIG_FILES([lv2/plugins/print.lv2/Makefile])
AC_CONFIG_FILES([lv2/plugins/metro.lv2/Makefile])
AC_CONFIG_FILES([lv2/plugins/bang.lv2/Makefile])
AC_CONFIG_FILES([lv2/plugins/transpose.lv2/Makefile])

# Redlandmm
AC_CONFIG_FILES([redlandmm/Makefile])
AC_CONFIG_FILES([redlandmm/src/Makefile])
AC_CONFIG_FILES([redlandmm/redlandmm/Makefile])
AC_CONFIG_FILES([redlandmm/tests/Makefile])
AC_CONFIG_FILES([redlandmm/doc/Makefile])
AC_CONFIG_FILES([redlandmm/doc/reference.doxygen])
AC_CONFIG_FILES([redlandmm/redlandmm.pc])

# RAUL
AC_CONFIG_FILES([raul/Makefile])
AC_CONFIG_FILES([raul/src/Makefile])
AC_CONFIG_FILES([raul/raul/Makefile])
AC_CONFIG_FILES([raul/tests/Makefile])
AC_CONFIG_FILES([raul/doc/Makefile])
AC_CONFIG_FILES([raul/doc/reference.doxygen])
AC_CONFIG_FILES([raul/raul.pc])

# FlowCanvas
AC_CONFIG_FILES([flowcanvas/Makefile])
AC_CONFIG_FILES([flowcanvas/src/Makefile])
AC_CONFIG_FILES([flowcanvas/flowcanvas/Makefile])
AC_CONFIG_FILES([flowcanvas/doc/Makefile])
AC_CONFIG_FILES([flowcanvas/doc/reference.doxygen])
AC_CONFIG_FILES([flowcanvas/flowcanvas.pc])

# Patchage
AC_CONFIG_FILES([patchage/Makefile])
AC_CONFIG_FILES([patchage/patchage.desktop])
AC_CONFIG_FILES([patchage/src/Makefile])
AC_CONFIG_FILES([patchage/icons/Makefile])
AC_CONFIG_FILES([patchage/icons/16x16/Makefile])
AC_CONFIG_FILES([patchage/icons/22x22/Makefile])
AC_CONFIG_FILES([patchage/icons/24x24/Makefile])
AC_CONFIG_FILES([patchage/icons/32x32/Makefile])
AC_CONFIG_FILES([patchage/icons/48x48/Makefile])
AC_CONFIG_FILES([patchage/icons/scalable/Makefile])

# Ingen
AC_CONFIG_FILES([ingen/Makefile])
AC_CONFIG_FILES([ingen/src/Makefile])
AC_CONFIG_FILES([ingen/src/common/Makefile])
AC_CONFIG_FILES([ingen/src/common/interface/Makefile])
AC_CONFIG_FILES([ingen/src/common/lv2ext/Makefile])
AC_CONFIG_FILES([ingen/src/libs/Makefile])
AC_CONFIG_FILES([ingen/src/libs/module/Makefile])
AC_CONFIG_FILES([ingen/src/libs/engine/Makefile])
AC_CONFIG_FILES([ingen/src/libs/engine/events/Makefile])
AC_CONFIG_FILES([ingen/src/libs/serialisation/Makefile])
AC_CONFIG_FILES([ingen/src/libs/client/Makefile])
AC_CONFIG_FILES([ingen/src/libs/gui/Makefile])
AC_CONFIG_FILES([ingen/src/bindings/Makefile])
AC_CONFIG_FILES([ingen/src/progs/Makefile])
AC_CONFIG_FILES([ingen/src/progs/ingen/Makefile])
AC_CONFIG_FILES([ingen/src/progs/supercollider/Makefile])
AC_CONFIG_FILES([ingen/src/progs/python/Makefile])
AC_CONFIG_FILES([ingen/src/progs/python/scripts/Makefile])
AC_CONFIG_FILES([ingen/src/progs/ingen/ingen.desktop])
AC_CONFIG_FILES([ingen/patches/Makefile])
AC_CONFIG_FILES([ingen/doc/Doxyfile])

# Machina
AC_CONFIG_FILES([machina/Makefile])
AC_CONFIG_FILES([machina/src/Makefile])
AC_CONFIG_FILES([machina/src/engine/Makefile])
AC_CONFIG_FILES([machina/src/engine/machina/Makefile])
AC_CONFIG_FILES([machina/src/gui/Makefile])
AC_CONFIG_FILES([machina/util/Makefile])
AC_CONFIG_FILES([machina/machina.desktop])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT

AC_MSG_RESULT([])
AC_MSG_RESULT([])
AC_MSG_RESULT([**********************************************************************])
AC_MSG_RESULT([Configuration:])
AC_MSG_RESULT([])
AC_MSG_RESULT([Jack support:            $build_jack])
AC_MSG_RESULT([LV2 Plugin support:      $build_lv2])
AC_MSG_RESULT([LADSPA Plugin support:   $build_ladspa])
AC_MSG_RESULT([LASH support:            $build_lash])
AC_MSG_RESULT([OSC support (via liblo): $build_lash])
AC_MSG_RESULT([])
AC_MSG_RESULT([LADSPA plugin install location: $ladspadir])
AC_MSG_RESULT([LV2 plugin install location:    $lv2plugindir])
AC_MSG_RESULT([])
AC_MSG_RESULT([FlowCanvas:])
AC_MSG_RESULT([    Anti-Aliasing: $anti_alias])
AC_MSG_RESULT([])
AC_MSG_RESULT([Ingen:])
AC_MSG_RESULT([    Engine library/OSC server: $build_ingen_server])
AC_MSG_RESULT([    JACK in-process engine:    $build_in_process_engine])
AC_MSG_RESULT([    Client library:            $build_ingen_client_lib])
AC_MSG_RESULT([    GTK GUI:                   $build_ingen_gtk_client])
AC_MSG_RESULT([])
AC_MSG_RESULT([Build unit tests: $build_unit_tests])
AC_MSG_RESULT([])
AC_MSG_RESULT([*** INGEN IS WITHOUT LASH SUPPORT AT THIS TIME ***])
AC_MSG_RESULT([])
AC_MSG_RESULT([C FLAGS:   $CFLAGS])
AC_MSG_RESULT([C++ FLAGS: $CXXFLAGS])
AC_MSG_RESULT([Please note the above and see README for performance information.])
AC_MSG_RESULT([**********************************************************************])
AC_MSG_RESULT([])

