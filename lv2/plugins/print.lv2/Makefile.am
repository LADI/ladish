AM_LDFLAGS = -shared -module -avoid-version -Wc,-nostartfiles
AM_CFLAGS = -shared -fPIC -DPIC @LV2CORE_CFLAGS@ @DROBINS_CFLAGS@

pluginbasename = print

plugindir   = @lv2plugindir@/$(pluginbasename).lv2
plugin_DATA = manifest.ttl $(pluginbasename).ttl
EXTRA_DIST = $(plugin_DATA)

uninstall-hook:
	rm -rf $(plugindir)/

plugin_LTLIBRARIES = print.la
print_la_SOURCES = print.c
print_la_LIBS = @LV2_OSC_LIBS@

