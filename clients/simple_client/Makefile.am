include $(top_srcdir)/common.am

bin_PROGRAMS = lash_simple_client

lash_simple_client_LDADD = \
	$(top_builddir)/liblash/liblash.la \
	$(JACK_LIBS) \
	$(ALSA_LIBS)

if LASH_OLD_API

lash_simple_client_SOURCES = \
	simple_client.c

lash_simple_client_CFLAGS = \
	$(JACK_CFLAGS) \
	$(ALSA_CFLAGS)

bin_PROGRAMS += lash_simple_client_newapi

lash_simple_client_newapi_SOURCES = \
	simple_client_newapi.c

lash_simple_client_newapi_CFLAGS = \
	$(JACK_CFLAGS) \
	$(ALSA_CFLAGS) \
	-DDEBUG_OUTPUT_TERMINAL

lash_simple_client_newapi_LDADD = $(lash_simple_client_LDADD)

else

lash_simple_client_SOURCES = \
	simple_client_newapi.c

lash_simple_client_CFLAGS = \
	$(JACK_CFLAGS) \
	$(ALSA_CFLAGS) \
	-DDEBUG_OUTPUT_TERMINAL

endif
