include $(top_srcdir)/common.am

if LASH_OLD_API
SUBDIRS = shell save_button panel
if HAVE_ALSA
SUBDIRS += simple_client synth
endif
else
if HAVE_ALSA
SUBDIRS = simple_client
endif
endif

dist_bin_SCRIPTS = lash_control
