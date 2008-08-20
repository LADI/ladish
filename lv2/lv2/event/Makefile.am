specbasename = lv2_event

specdir   = @lv2plugindir@/$(specbasename).lv2
spec_DATA = manifest.ttl $(specbasename).ttl
spec_HEADERS = $(specbasename).h
EXTRA_DIST = $(spec_DATA) $(spec_HEADERS)

uninstall-hook:
	rm -rf $(specdir)/

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = lv2_event.pc

lv2eventincludedir = $(includedir)/lv2/event
lv2eventinclude_HEADERS = lv2_event.h lv2_event_helpers.h
