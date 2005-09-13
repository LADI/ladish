include $(top_srcdir)/common.am

EXTRA_PROGRAMS = lash_gtk_client

lash_gtk_client_SOURCES = \
	main.c \
	gtk_client.c gtk_client.h

lash_gtk_client_CFLAGS = \
	$(LASH_CFLAGS) \
	$(GTK2_CFLAGS)

lash_gtk_client_LDADD = \
	$(top_builddir)/liblash/liblash.la \
	$(GTK2_LIBS)

if HAVE_GTK2
  bin_PROGRAMS = lash_gtk_client
endif