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

SUBDIRS = flowcanvas src $(DOC_DIR) debian
DIST_SUBDIRS = flowcanvas src doc debian

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = flowcanvas.pc

dist-hook: dist-check-doxygen

docs:
	cd doc && make docs

deb:
	dpkg-buildpackage -sn
