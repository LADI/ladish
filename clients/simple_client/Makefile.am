include $(top_srcdir)/common.am

bin_PROGRAMS = lash_simple_client

lash_simple_client_SOURCES = \
	simple_client.c

lash_simple_client_CFLAGS = \
	$(JACK_CFLAGS) \
	$(ALSA_CFLAGS)

lash_simple_client_LDADD = \
	$(top_builddir)/liblash/liblash.la \
	$(JACK_LIBS) \
	$(ALSA_LIBS)
