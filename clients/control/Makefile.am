include $(top_srcdir)/common.am

EXTRA_PROGRAMS = lash_control

lash_control_SOURCES = \
	main.c \
	lash_control.c \
	lash_control.h \
	client.c client.h \
	project.c \
	project.h

lash_control_CFLAGS = \
	$(LASH_CFLAGS)

lash_control_LDADD = \
	$(top_builddir)/liblash/liblash.la \
	$(LASH_LIBS) \
	$(READLINE_LIBS)


if HAVE_READLINE
  bin_PROGRAMS = lash_control
endif
