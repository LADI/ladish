AM_CXXFLAGS = -I$(top_srcdir)/flowcanvas -I$(top_srcdir)/raul -I$(top_srcdir)/machina/src/engine -DPKGDATADIR=\"$(pkgdatadir)\" @LIBGLADEMM_CFLAGS@ @GNOMECANVASMM_CFLAGS@ @JACK_CFLAGS@

EXTRA_DIST = machina.gladep

sharefilesdir = $(pkgdatadir)
dist_sharefiles_DATA = machina.glade machina.svg

globalpixmapsdir = $(datadir)/pixmaps
dist_globalpixmaps_DATA = machina.svg

machina_gui_SOURCES = \
	main.cpp \
	GladeXml.hpp \
	MachinaGUI.hpp \
	MachinaGUI.cpp \
	MachinaCanvas.hpp \
	MachinaCanvas.cpp \
	NodeView.hpp \
	NodeView.cpp \
	EdgeView.hpp \
	EdgeView.cpp \
	NodePropertiesWindow.hpp \
	NodePropertiesWindow.cpp

if WITH_MACHINA_GUI

machina_gui_LDADD = ../engine/libmachina.la @RAUL_LIBS@ @FLOWCANVAS_LIBS@ @LIBGLADEMM_LIBS@ @GNOMECANVASMM_LIBS@ @JACK_LIBS@

bin_PROGRAMS = machina_gui

endif
