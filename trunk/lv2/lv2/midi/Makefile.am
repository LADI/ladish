specbasename = lv2_midi

specdir   = @lv2plugindir@/$(specbasename).lv2
spec_DATA = manifest.ttl $(specbasename).ttl
EXTRA_DIST = $(spec_DATA)

uninstall-hook:
	rm -rf $(specdir)/

