AM_CXXFLAGS = \
	-DPKGDATADIR=\"$(pkgdatadir)\" \
	-I$(top_srcdir)/flowcanvas \
	-I$(top_srcdir)/machina/src/engine \
	-I$(top_srcdir)/raul \
	@GNOMECANVASMM_CFLAGS@ \
	@GTHREAD_CFLAGS@ \
	@JACK_CFLAGS@ \
	@LIBGLADEMM_CFLAGS@ \
	@RAUL_CFLAGS@ \
	@REDLANDMM_CFLAGS@

EXTRA_DIST = machina.gladep

sharefilesdir = $(pkgdatadir)
dist_sharefiles_DATA = machina.glade machina.svg

globalpixmapsdir = $(datadir)/pixmaps
dist_globalpixmaps_DATA = machina.svg

machina_gui_SOURCES = \
	EdgeView.cpp \
	EdgeView.hpp \
	GladeXml.hpp \
	MachinaCanvas.cpp \
	MachinaCanvas.hpp \
	MachinaGUI.cpp \
	MachinaGUI.hpp \
	NodePropertiesWindow.cpp \
	NodePropertiesWindow.hpp \
	NodeView.cpp \
	NodeView.hpp \
	main.cpp

if WITH_MACHINA_GUI

machina_gui_LDADD = \
	../engine/libmachina.la \
	@FLOWCANVAS_LIBS@ \
	@GNOMECANVASMM_LIBS@ \
	@JACK_LIBS@ \
	@LIBGLADEMM_LIBS@ \
	@RAUL_LIBS@ \
	@REDLANDMM_LIBS@

bin_PROGRAMS = machina_gui

endif
