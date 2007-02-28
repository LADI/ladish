DIST_SUBDIRS = util
SUBDIRS = src

desktopfilesdir = $(datadir)/applications
dist_desktopfiles_DATA = machina.desktop

doc: Doxyfile src/*.h src/*.cpp
	doxygen
