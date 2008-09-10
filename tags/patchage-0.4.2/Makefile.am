SUBDIRS = src icons debian-sid

desktopfilesdir = $(datadir)/applications
dist_desktopfiles_DATA = patchage.desktop

docs: Doxyfile src/*.h src/*.cpp
	doxygen

deb:
	if [ -e debian ]; then \
		echo "ERROR: ./debian exists, you must move it out of the way."; \
	else \
		ln -s debian-sid debian; \
		dpkg-buildpackage -sn; \
		rm debian; \
	fi
