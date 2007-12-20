AM_CXXFLAGS = -I.. -I$(top_srcdir)/raul -I$(top_srcdir)/flowcanvas -DDATA_DIR=\"$(pkgdatadir)\" @GTHREAD_CFLAGS@ @LIBGLADEMM_CFLAGS@ @GNOMECANVASMM_CFLAGS@ @JACK_CFLAGS@ @ALSA_CFLAGS@ @LASH_CFLAGS@
patchage_LDADD = @GTHREAD_LIBS@ @LIBGLADEMM_LIBS@ @GNOMECANVASMM_LIBS@ @JACK_LIBS@ @ALSA_LIBS@ @LASH_LIBS@ @RAUL_LIBS@ @FLOWCANVAS_LIBS@

EXTRA_DIST = patchage.gladep

sharefilesdir = $(pkgdatadir)
dist_sharefiles_DATA = patchage.glade

bin_PROGRAMS = patchage
patchage_SOURCES = \
	Driver.hpp \
	GladeFile.hpp \
	JackDriver.cpp \
	JackDriver.hpp \
	JackSettingsDialog.hpp \
	Patchage.cpp \
	Patchage.hpp \
	PatchageCanvas.cpp \
	PatchageCanvas.hpp \
	PatchageEvent.cpp \
	PatchageEvent.hpp \
	PatchageModule.hpp \
	PatchagePort.hpp \
	StateManager.cpp  \
	StateManager.hpp \
	Widget.hpp \
	main.cpp

if WITH_LASH
patchage_SOURCES += LashDriver.hpp LashDriver.cpp
endif

if WITH_ALSA
patchage_SOURCES += AlsaDriver.hpp AlsaDriver.cpp
endif
