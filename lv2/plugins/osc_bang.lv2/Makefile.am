AM_LDFLAGS = -shared -module -avoid-version -Wc,-nostartfiles @GTK_LIBS@
AM_CFLAGS = -shared -fPIC -DPIC @GTK_CFLAGS@ @SLV2_CFLAGS@ -I$(top_srcdir)/lv2 @LV2CORE_CFLAGS@

pluginbasename = osc_bang
pluginuibasename = osc_bang_ui

plugindir   = @lv2plugindir@/$(pluginbasename).lv2
plugin_DATA = manifest.ttl $(pluginbasename).ttl
EXTRA_DIST = $(plugin_DATA)

uninstall-hook:
	rm -rf $(plugindir)/

plugin_LTLIBRARIES = osc_bang.la osc_bang_ui.la
osc_bang_la_SOURCES = osc_bang.c \
                      $(top_srcdir)/lv2/extensions/osc/lv2_osc.c \
                      $(top_srcdir)/lv2/extensions/osc/lv2_osc_print.c

osc_bang_ui_la_SOURCES = osc_bang_ui.c \
                         $(top_srcdir)/lv2/extensions/osc/lv2_osc.c \
                         $(top_srcdir)/lv2/extensions/osc/lv2_osc_print.c

