AM_LDFLAGS = -module -avoid-version -Wc,-nostartfiles

pluginbasename = osc_metro

plugindir   = @lv2plugindir@/$(pluginbasename).lv2
plugin_DATA = manifest.ttl $(pluginbasename).ttl
EXTRA_DIST = $(plugin_DATA)

uninstall-hook:
	rm -rf $(plugindir)/

plugin_LTLIBRARIES = osc_metro.la
osc_metro_la_SOURCES = osc_metro.c \
                       ../../extensions/osc/lv2_osc.c \
                       ../../extensions/osc/lv2_osc_print.c

