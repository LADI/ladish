AM_LDFLAGS = -shared -module -avoid-version -Wc,-nostartfiles @GTK_LIBS@
AM_CFLAGS = -shared -fPIC -DPIC @GTK_CFLAGS@ @LV2CORE_CFLAGS@ @DROBINS_CFLAGS@

pluginbasename = osc_bang
pluginuibasename = osc_bang_ui

plugindir   = @lv2plugindir@/$(pluginbasename).lv2
plugin_DATA = manifest.ttl $(pluginbasename).ttl
EXTRA_DIST = $(plugin_DATA)

uninstall-hook:
	rm -rf $(plugindir)/

plugin_LTLIBRARIES = osc_bang.la osc_bang_ui.la
osc_bang_la_SOURCES = osc_bang.c
osc_bang_la_SOURCES += @DROBINS_SRCDIR@/extensions/osc/lv2_osc.c

osc_bang_ui_la_SOURCES = osc_bang_ui.c
osc_bang_ui_la_SOURCES += @DROBINS_SRCDIR@/extensions/osc/lv2_osc.c

