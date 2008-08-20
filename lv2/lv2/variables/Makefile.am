specbasename = lv2_variables

specdir   = @lv2plugindir@/$(specbasename).lv2
spec_DATA = manifest.ttl $(specbasename).ttl
spec_HEADERS = $(specbasename).h lv2_variables_private.h
EXTRA_DIST = $(spec_DATA)

uninstall-hook:
	rm -rf $(specdir)/

