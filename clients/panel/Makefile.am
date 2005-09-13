include $(top_srcdir)/common.am

EXTRA_PROGRAMS = lash_panel

lash_panel_SOURCES = \
	main.c \
	panel.h panel.c \
	project.h project.c

lash_panel_CFLAGS = \
	$(LASH_CFLAGS) \
	$(GTK2_CFLAGS)

lash_panel_LDADD = \
	$(top_builddir)/liblash/liblash.la \
	$(GTK2_LIBS)

if HAVE_GTK2
  bin_PROGRAMS = lash_panel
endif
