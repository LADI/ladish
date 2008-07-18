SUBDIRS = src icons

desktopfilesdir = $(datadir)/applications
dist_desktopfiles_DATA = patchage.desktop

docs: Doxyfile src/*.h src/*.cpp
	doxygen

deb:
	dpkg-buildpackage -sn
