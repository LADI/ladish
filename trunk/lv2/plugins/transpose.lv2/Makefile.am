AM_LDFLAGS = -shared -module -avoid-version -Wc,-nostartfiles @GTK_LIBS@
AM_CPPFLAGS = -shared -fPIC -DPIC @GTK_CFLAGS@ @LV2CORE_CFLAGS@ @DROBINS_CFLAGS@

pluginbasename = transpose

plugindir   = @lv2plugindir@/$(pluginbasename).lv2
plugin_DATA = manifest.ttl $(pluginbasename).ttl
EXTRA_DIST = $(plugin_DATA)

uninstall-hook:
	rm -rf $(plugindir)/

plugin_LTLIBRARIES = transpose.la
transpose_la_SOURCES = transpose.cpp
transpose_la_SOURCES += ../../ext/lv2plugin.cpp

