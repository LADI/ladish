AM_LDFLAGS = -module -avoid-version -Wc,-nostartfiles

pluginbasename = osc_print

plugindir   = @lv2plugindir@/$(pluginbasename).lv2
plugin_DATA = manifest.ttl $(pluginbasename).ttl
EXTRA_DIST = $(plugin_DATA)

uninstall-hook:
	rm -rf $(plugindir)/

plugin_LTLIBRARIES = osc_print.la
osc_print_la_SOURCES = osc_print.c \
                       ../../extensions/osc/lv2_osc.c \
                       ../../extensions/osc/lv2_osc_print.c

