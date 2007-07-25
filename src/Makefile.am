AM_CXXFLAGS = -I.. -I$(top_srcdir)/raul -I$(top_srcdir)/flowcanvas -DPKGDATADIR=\"$(pkgdatadir)\" @GTHREAD_CFLAGS@ @LIBGLADEMM_CFLAGS@ @GNOMECANVASMM_CFLAGS@ @JACK_CFLAGS@ @ALSA_CFLAGS@ @LASH_CFLAGS@
patchage_LDADD = @GTHREAD_LIBS@ @LIBGLADEMM_LIBS@ @GNOMECANVASMM_LIBS@ @JACK_LIBS@ @ALSA_LIBS@ @LASH_LIBS@ @RAUL_LIBS@ @FLOWCANVAS_LIBS@

EXTRA_DIST = patchage.gladep

sharefilesdir = $(pkgdatadir)
dist_sharefiles_DATA = patchage.glade patchage.svg

globalpixmapsdir = $(datadir)/pixmaps
dist_globalpixmaps_DATA = patchage.svg

bin_PROGRAMS = patchage
patchage_SOURCES = \
	main.cpp \
	Patchage.hpp \
	Patchage.cpp \
	JackSettingsDialog.hpp \
	StateManager.hpp \
	StateManager.cpp  \
	PatchageModule.hpp \
	PatchagePort.hpp \
	Driver.hpp \
	PatchageEvent.hpp \
	PatchageEvent.cpp \
	JackDriver.hpp \
	JackDriver.cpp \
	PatchageCanvas.hpp \
	PatchageCanvas.cpp

if WITH_LASH
patchage_SOURCES += LashDriver.hpp LashDriver.cpp
endif

if WITH_ALSA
patchage_SOURCES += AlsaDriver.hpp AlsaDriver.cpp
endif
