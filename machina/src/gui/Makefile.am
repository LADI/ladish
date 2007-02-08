AM_CXXFLAGS = -DPKGDATADIR=\"$(pkgdatadir)\" @LIBGLADEMM_CFLAGS@ @GNOMECANVASMM_CFLAGS@ @JACK_CFLAGS@ @FLOWCANVAS_CFLAGS@ @RAUL_CFLAGS@
machina_gui_LDADD = @LIBGLADEMM_LIBS@ @GNOMECANVASMM_LIBS@ @JACK_LIBS@ @FLOWCANVAS_LIBS@ @RAUL_CFLAGS@

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
	MachinaCanvas.cpp

endif
