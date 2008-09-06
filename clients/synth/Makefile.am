include $(top_srcdir)/common.am

bin_PROGRAMS = lash_synth

lash_synth_SOURCES = \
	main.c \
	synth.c synth.h \
	interface.c interface.h \
	lash.c lash.h

lash_synth_CFLAGS = \
	$(JACK_CFLAGS) \
	$(ALSA_CFLAGS) \
	$(GTK2_CFLAGS) \
	-D_POSIX_PTHREAD_SEMANTICS

lash_synth_LDADD = \
	$(top_builddir)/liblash/liblash.la \
	$(JACK_LIBS) \
	$(ALSA_LIBS) \
	$(GTK2_LIBS) \
	-lpthread
