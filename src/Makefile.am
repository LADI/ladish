AM_CXXFLAGS = -I.. -DDATA_DIR=\"$(pkgdatadir)\" @GTHREAD_CFLAGS@ @LIBGLADEMM_CFLAGS@ @GNOMECANVASMM_CFLAGS@
patchage_LDADD = @GTHREAD_LIBS@ @LIBGLADEMM_LIBS@ @GNOMECANVASMM_LIBS@ @FLOWCANVAS_LIBS@

EXTRA_DIST = patchage.gladep

sharefilesdir = $(pkgdatadir)
dist_sharefiles_DATA = patchage.glade

bin_PROGRAMS = patchage
patchage_SOURCES = \
	GladeFile.hpp \
	JackSettingsDialog.hpp \
	Patchage.cpp \
	Patchage.hpp \
	PatchageCanvas.cpp \
	PatchageCanvas.hpp \
	PatchageModule.hpp \
	PatchagePort.hpp \
	StateManager.cpp  \
	StateManager.hpp \
	Widget.hpp \
	main.cpp

patchage_SOURCES += common.hpp
patchage_SOURCES += globals.hpp

AM_CXXFLAGS += @DBUS_CFLAGS@
patchage_SOURCES += jack_proxy.cpp
patchage_SOURCES += jack_proxy.hpp
patchage_LDADD +=  @DBUS_LIBS@

patchage_SOURCES += lash_proxy.cpp
patchage_SOURCES += lash_proxy.hpp

patchage_SOURCES += project_list.cpp
patchage_SOURCES += project_list.hpp

patchage_SOURCES += project.cpp
patchage_SOURCES += project.hpp

patchage_SOURCES += session.cpp
patchage_SOURCES += session.hpp

patchage_SOURCES += lash_client.cpp
patchage_SOURCES += lash_client.hpp

patchage_SOURCES += project_properties.cpp
patchage_SOURCES += project_properties.hpp
