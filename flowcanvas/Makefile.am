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

SUBDIRS = flowcanvas src $(DOC_DIR)
DIST_SUBDIRS = flowcanvas src doc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = flowcanvas.pc

dist-hook: dist-check-doxygen

docs:
	cd doc && make docs
