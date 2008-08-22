AM_CXXFLAGS = -I.. -I$(top_srcdir)/raul -I$(top_srcdir)/flowcanvas -DDATA_DIR=\"$(pkgdatadir)\" @GTHREAD_CFLAGS@ @LIBGLADEMM_CFLAGS@ @GNOMECANVASMM_CFLAGS@ @ALSA_CFLAGS@ @DBUS_CFLAGS@
patchage_LDADD = @GTHREAD_LIBS@ @LIBGLADEMM_LIBS@ @GNOMECANVASMM_LIBS@ @ALSA_LIBS@ @RAUL_LIBS@ @FLOWCANVAS_LIBS@ @DBUS_LIBS@

EXTRA_DIST = patchage.gladep

sharefilesdir = $(pkgdatadir)
dist_sharefiles_DATA = patchage.glade

bin_PROGRAMS = patchage
patchage_SOURCES = \
	Driver.hpp \
	GladeFile.hpp \
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

if WITH_JACK_DBUS
AM_CXXFLAGS += @DBUS_CFLAGS@
patchage_SOURCES += JackDbusDriver.cpp
patchage_SOURCES += JackDbusDriver.hpp
patchage_LDADD +=  @DBUS_LIBS@
else
if WITH_JACK
AM_CXXFLAGS += @JACK_CFLAGS@
patchage_SOURCES += JackDriver.cpp
patchage_SOURCES += JackDriver.hpp
patchage_LDADD +=  @JACK_LIBS@
endif
endif

if WITH_DBUS
patchage_SOURCES += \
	DBus.cpp \
	DBus.hpp \
	LashClient.cpp \
	LashClient.hpp \
	LashProxy.cpp \
	LashProxy.hpp \
	LoadProjectDialog.cpp \
	LoadProjectDialog.hpp \
	Project.cpp \
	Project.hpp \
	ProjectList.cpp \
	ProjectList.hpp \
	ProjectPropertiesDialog.cpp \
	ProjectPropertiesDialog.hpp \
	Session.cpp \
	Session.hpp
endif

if WITH_ALSA
patchage_SOURCES += AlsaDriver.hpp AlsaDriver.cpp
endif
