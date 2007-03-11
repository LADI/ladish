AM_CXXFLAGS = -DPKGDATADIR=\"$(pkgdatadir)\" @LIBGLADEMM_CFLAGS@ @GNOMECANVASMM_CFLAGS@ @JACK_CFLAGS@ @FLOWCANVAS_CFLAGS@ @RAUL_CFLAGS@  -I$(top_srcdir)/src/engine
machina_gui_LDADD = @FLOWCANVAS_LIBS@ @LIBGLADEMM_LIBS@ @GNOMECANVASMM_LIBS@ @JACK_LIBS@ @RAUL_CFLAGS@ ../engine/libmachina.la

EXTRA_DIST = machina.gladep

sharefilesdir = $(pkgdatadir)
dist_sharefiles_DATA = machina.glade

#globalpixmapsdir = $(datadir)/pixmaps
#dist_globalpixmaps_DATA = machina-icon.svg

if WITH_GUI

bin_PROGRAMS = machina_gui
machina_gui_SOURCES = \
	main.cpp \
	MachinaGUI.hpp \
	MachinaGUI.cpp \
	MachinaCanvas.hpp \
	MachinaCanvas.cpp \
	NodeView.hpp \
	NodeView.cpp \
	EdgeView.hpp \
	EdgeView.cpp

endif
