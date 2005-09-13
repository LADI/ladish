include $(top_srcdir)/common.am

EXTRA_PROGRAMS = lash_save_button_2 lash_save_button

lash_save_button_2_SOURCES = \
	save_button_2.0.c

lash_save_button_2_CFLAGS = \
	$(LASH_CFLAGS) \
	$(GTK2_CFLAGS)

lash_save_button_2_LDADD = \
	$(top_builddir)/liblash/liblash.la \
	$(GTK2_LIBS)


lash_save_button_SOURCES = \
	save_button.c \
	hints.c hints.h

lash_save_button_CFLAGS = \
	$(LASH_CFLAGS) \
	$(GTK_CFLAGS)

lash_save_button_LDADD = \
	$(top_builddir)/liblash/liblash.la \
	$(GTK_LIBS)

if HAVE_GTK2
if HAVE_GTK
  bin_PROGRAMS = lash_save_button_2 lash_save_button
else
  bin_PROGRAMS = lash_save_button_2
endif
else
if HAVE_GTK
  bin_PROGRAMS = lash_save_button
endif
endif
