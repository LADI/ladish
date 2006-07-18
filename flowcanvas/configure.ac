AC_PREREQ(2.59)
AC_INIT([flowcanvas],[0.1.0],[drobilla@connect.carleton.ca])
AC_CONFIG_SRCDIR([src/FlowCanvas.cpp])
AC_CONFIG_SRCDIR([flowcanvas/FlowCanvas.h])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

# Checks for compiler
AC_PROG_CXX

# Check pedantic other stuff autoscan says I should :)
AC_C_CONST
AC_C_INLINE
AC_HEADER_STDBOOL
AC_TYPE_SIZE_T

# Library building stuff
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

# Check for GTKMM
PKG_CHECK_MODULES(GTKMM, gtkmm-2.4)
AC_SUBST(GTKMM_CFLAGS)
AC_SUBST(GTKMM_LIBS)

# Check for gnomecanvasmm
PKG_CHECK_MODULES(GNOMECANVASMM, libgnomecanvasmm-2.6)
AC_SUBST(GNOMECANVASMM_CFLAGS)
AC_SUBST(GNOMECANVASMM_LIBS)

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

# Write it!
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([flowcanvas/Makefile])
AC_CONFIG_FILES([doc/Makefile])
AC_CONFIG_FILES([flowcanvas.pc])
AC_OUTPUT
