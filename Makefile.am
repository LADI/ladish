SUBDIRS = src

globalpixmapsdir = $(datadir)/pixmaps
dist_globalpixmaps_DATA = patchage-icon.svg

desktopfilesdir = $(datadir)/applications
dist_desktopfiles_DATA = patchage.desktop

doc: Doxyfile src/*.h src/*.cpp
	doxygen
