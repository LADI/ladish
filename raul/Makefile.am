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

SUBDIRS = raul src $(DOC_DIR)
DIST_SUBDIRS = raul src tests doc

if BUILD_UNIT_TESTS
SUBDIRS += tests
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = raul.pc

dist-hook: dist-check-doxygen

docs:
	cd doc && make docs
