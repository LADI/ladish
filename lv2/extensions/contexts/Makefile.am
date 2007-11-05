AM_LDFLAGS = -module -avoid-version -Wc,-nostartfiles

specbasename = lv2_contexts

specdir   = @lv2plugindir@/$(specbasename).lv2
spec_DATA = manifest.ttl $(specbasename).ttl
EXTRA_DIST = $(spec_DATA) lv2_contexts.h

uninstall-hook:
	rm -rf $(specdir)/

