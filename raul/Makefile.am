if HAVE_DOXYGEN
DOC_DIR = doc
dist-check-doxygen:
else
DOC_DIR =
dist-check-doxygen:
	@echo
	@echo ' ******' You need doxygen installed to make dist.' ******'
	@echo
	@false
endif

SUBDIRS = raul src $(DOC_DIR) debian-sid
DIST_SUBDIRS = raul src tests doc debian-sid

if BUILD_UNIT_TESTS
SUBDIRS += tests
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = raul.pc

dist-hook: dist-check-doxygen

docs:
	cd doc && make docs

deb:
	if [ -e debian ]; then \
		echo "ERROR: ./debian exists, you must move it out of the way. \
	else \
		ln -s debian-sid debian \
		dpkg-buildpackage -sn \
		rm debian
	fi
