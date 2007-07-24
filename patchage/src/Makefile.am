AM_CXXFLAGS = -I.. -I$(top_srcdir)/raul -I$(top_srcdir)/flowcanvas -DPKGDATADIR=\"$(pkgdatadir)\" @GTHREAD_CFLAGS@ @LIBGLADEMM_CFLAGS@ @GNOMECANVASMM_CFLAGS@ @JACK_CFLAGS@ @ALSA_CFLAGS@ @LASH_CFLAGS@
patchage_LDADD = @RAUL_LIBS@ @FLOWCANVAS_LIBS@ @GTHREAD_LIBS@ @LIBGLADEMM_LIBS@ @GNOMECANVASMM_LIBS@ @JACK_LIBS@ @ALSA_LIBS@ @LASH_LIBS@

EXTRA_DIST = patchage.gladep

sharefilesdir = $(pkgdatadir)
dist_sharefiles_DATA = patchage.glade patchage.svg

globalpixmapsdir = $(datadir)/pixmaps
dist_globalpixmaps_DATA = patchage.svg

bin_PROGRAMS = patchage
patchage_SOURCES = \
	main.cpp \
	Patchage.h \
	Patchage.cpp \
	JackSettingsDialog.h \
	StateManager.h \
	StateManager.cpp  \
	PatchageModule.h \
	PatchagePort.h \
	Driver.h \
	PatchageEvent.h \
	PatchageEvent.cpp \
	JackDriver.h \
	JackDriver.cpp \
	PatchageCanvas.h \
	PatchageCanvas.cpp

if WITH_LASH
patchage_SOURCES += LashDriver.h LashDriver.cpp
endif

if WITH_ALSA
patchage_SOURCES += AlsaDriver.h AlsaDriver.cpp
endif
