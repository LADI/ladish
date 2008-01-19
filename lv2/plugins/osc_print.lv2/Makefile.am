AM_LDFLAGS = -shared -module -avoid-version -Wc,-nostartfiles
AM_CFLAGS = -shared -fPIC -DPIC @LV2CORE_CFLAGS@ @DROBINS_CFLAGS@

pluginbasename = osc_print

plugindir   = @lv2plugindir@/$(pluginbasename).lv2
plugin_DATA = manifest.ttl $(pluginbasename).ttl
EXTRA_DIST = $(plugin_DATA)

uninstall-hook:
	rm -rf $(plugindir)/

plugin_LTLIBRARIES = osc_print.la
osc_print_la_SOURCES = osc_print.c
osc_print_la_SOURCES += @DROBINS_SRCDIR@/lv2/osc/lv2_osc.c
osc_print_la_SOURCES += @DROBINS_SRCDIR@/lv2/osc/lv2_osc_print.c

