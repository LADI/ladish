SUBDIRS = redlandmm src doc

if BUILD_UNIT_TESTS
SUBDIRS += tests
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = redlandmm.pc

docs:
	cd doc && make docs
