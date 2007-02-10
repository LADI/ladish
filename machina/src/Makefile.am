#globalpixmapsdir = $(datadir)/pixmaps
#dist_globalpixmaps_DATA = machina-icon.svg

SUBDIRS = engine gui

bin_PROGRAMS = machina

machina_CXXFLAGS = @RAUL_CFLAGS@ @JACK_CFLAGS@ @GLIBMM_CFLAGS@ -I$(top_srcdir)/src/engine
machina_LDADD    = @RAUL_LIBS@   @JACK_LIBS@   @GLIBMM_LIBS@ engine/libmachina.la

machina_SOURCES = main.cpp
