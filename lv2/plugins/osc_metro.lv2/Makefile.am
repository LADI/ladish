AM_LDFLAGS = -module -avoid-version -Wc,-nostartfiles
AM_CFLAGS = -shared -fPIC -DPIC -I$(top_srcdir)/lv2

pluginbasename = osc_metro

plugindir   = @lv2plugindir@/$(pluginbasename).lv2
plugin_DATA = manifest.ttl $(pluginbasename).ttl
EXTRA_DIST = $(plugin_DATA)

uninstall-hook:
	rm -rf $(plugindir)/

plugin_LTLIBRARIES = osc_metro.la
osc_metro_la_SOURCES = osc_metro.c \
                       $(top_srcdir)/lv2/extensions/osc/lv2_osc.c \
                       $(top_srcdir)/lv2/extensions/osc/lv2_osc_print.c

