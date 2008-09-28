AM_LDFLAGS = -shared -module -avoid-version -Wc,-nostartfiles @GTK_LIBS@
AM_CFLAGS = -shared -fPIC -DPIC \
			@LV2CORE_CFLAGS@ @DROBINS_CFLAGS@ @LV2_OSC_CFLAGS@ @GTK_CFLAGS@

pluginbasename = bang
pluginuibasename = bang_ui

plugindir   = @lv2plugindir@/$(pluginbasename).lv2
plugin_DATA = manifest.ttl $(pluginbasename).ttl
EXTRA_DIST = $(plugin_DATA)

uninstall-hook:
	rm -rf $(plugindir)/

plugin_LTLIBRARIES = bang.la bang_ui.la

bang_la_SOURCES = bang.c
bang_la_LIBS = @LV2_OSC_LIBS@

bang_ui_la_SOURCES = bang_ui.c
bang_ui_la_LIBS = @LV2_OSC_LIBS@

