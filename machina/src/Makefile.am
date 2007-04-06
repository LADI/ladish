SUBDIRS = engine gui

bin_PROGRAMS = midi2machina

if WITH_JACK
bin_PROGRAMS += machina
endif

machina_SOURCES = machina.cpp

AM_CXXFLAGS = @RAUL_CFLAGS@ @JACK_CFLAGS@ @GLIBMM_CFLAGS@ -I$(top_srcdir)/src/engine
LDADD    = @RAUL_LIBS@   @JACK_LIBS@   @GLIBMM_LIBS@ engine/libmachina.la

midi2machina_SOURCES = midi2machina.cpp
