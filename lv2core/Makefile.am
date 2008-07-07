AUTOMAKE_OPTIONS = foreign

SUBDIRS = lv2core.lv2 debian

lv2includedir = $(includedir)
lv2include_HEADERS = lv2.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = lv2core.pc

deb:
	dpkg-buildpackage -sn
