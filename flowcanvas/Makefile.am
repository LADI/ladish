SUBDIRS = flowcanvas src doc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = flowcanvas.pc

doc: Doxyfile src/*.cpp flowcanvas/*.h
	doxygen
