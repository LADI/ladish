AM_LDFLAGS = -module -avoid-version -Wc,-nostartfiles
AM_CFLAGS = -shared -fPIC -DPIC -I$(top_srcdir)/lv2 \
			@LV2CORE_CFLAGS@ @DROBINS_CFLAGS@ @LV2_OSC_CFLAGS@

pluginbasename = metro

plugindir   = @lv2plugindir@/$(pluginbasename).lv2
plugin_DATA = manifest.ttl $(pluginbasename).ttl
EXTRA_DIST = $(plugin_DATA)

uninstall-hook:
	rm -rf $(plugindir)/

plugin_LTLIBRARIES = metro.la
metro_la_SOURCES = metro.c
metro_la_LIBS = @LV2_OSC_LIBS@

