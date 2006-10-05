AM_CXXFLAGS = -DPKGDATADIR=\"$(pkgdatadir)\" @LIBGLADEMM_CFLAGS@ @GNOMECANVASMM_CFLAGS@ @JACK_CFLAGS@ @ALSA_CFLAGS@ @LASH_CFLAGS@ @FLOWCANVAS_CFLAGS@ @RAUL_CFLAGS@
patchage_LDADD = @LIBGLADEMM_LIBS@ @GNOMECANVASMM_LIBS@ @JACK_LIBS@ @ALSA_LIBS@ @LASH_LIBS@ @FLOWCANVAS_LIBS@ @RAUL_CFLAGS@

EXTRA_DIST = patchage.gladep

sharefilesdir = $(pkgdatadir)
dist_sharefiles_DATA = patchage.glade

bin_PROGRAMS = patchage
patchage_SOURCES = \
	main.cpp  \
	Patchage.h \
	Patchage.cpp \
	StateManager.h \
	StateManager.cpp  \
	PatchageModule.h \
	PatchagePort.h \
	Driver.h \
	JackDriver.h \
	JackDriver.cpp \
	AlsaDriver.h \
	AlsaDriver.cpp \
	PatchageFlowCanvas.h \
	PatchageFlowCanvas.cpp

if WITH_LASH
patchage_SOURCES += LashDriver.h LashDriver.cpp
endif
