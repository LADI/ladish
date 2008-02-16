EXTRA_DIST = lv2_osc_test.c
AM_CFLAGS = -I../event

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = lv2_osc.pc

lv2oscincludedir = $(includedir)/lv2/osc
lv2oscinclude_HEADERS = lv2_osc.h lv2_osc_print.h

lib_LTLIBRARIES = liblv2_osc.la
liblv2_osc_la_LDFLAGS = -version-info '0:0:0'

liblv2_osc_la_SOURCES = \
	lv2_osc.c \
	lv2_osc_print.c
