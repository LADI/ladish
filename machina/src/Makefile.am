SUBDIRS = engine gui

bin_PROGRAMS = midi2machina

if WITH_JACK
bin_PROGRAMS += machina
endif

machina_SOURCES = machina.cpp

AM_CXXFLAGS = -I$(top_srcdir)/raul -I$(top_srcdir)/machina/src/engine @JACK_CFLAGS@ @GLIBMM_CFLAGS@
LDADD = @RAUL_LIBS@ @JACK_LIBS@ @GLIBMM_LIBS@ engine/libmachina.la

midi2machina_SOURCES = midi2machina.cpp
