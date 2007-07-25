AM_LDFLAGS = -module -avoid-version -Wc,-nostartfiles

plugindir   = @lv2plugindir@/$(pluginbasename).lv2
plugin_DATA = manifest.ttl $(pluginbasename).ttl

uninstall-hook:
	rm -r $(plugindir)/

pluginbasename = osc_metro

plugin_LTLIBRARIES = osc_metro.la
osc_metro_la_SOURCES = $(pluginbasename).c \
                       ../../extensions/osc/lv2_osc.c \
                       ../../extensions/osc/lv2_osc_print.c

