include $(top_srcdir)/common.am

EXTRA_PROGRAMS = lash_save_button

lash_save_button_SOURCES = \
	save_button.c

lash_save_button_CFLAGS = \
	$(GTK2_CFLAGS)

lash_save_button_LDADD = \
	$(top_builddir)/liblash/liblash.la \
	$(GTK2_LIBS)

if HAVE_GTK2
  bin_PROGRAMS = lash_save_button
endif
