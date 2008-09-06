include $(top_srcdir)/common.am

EXTRA_PROGRAMS = lash_shell

if HAVE_READLINE

bin_PROGRAMS = lash_shell

lash_shell_SOURCES = \
	main.c \
	lash_control.c \
	lash_control.h \
	client.c client.h \
	project.c \
	project.h

# Include safemem code
nodist_lash_shell_SOURCES = safety.c
BUILT_SOURCES = safety.c
CLEANFILES = safety.c
safety.c: Makefile
	cp $(top_srcdir)/common/safety.c .

lash_shell_CFLAGS = \
	-DDEBUG_OUTPUT_TERMINAL

lash_shell_LDADD = \
	$(top_builddir)/liblash/liblash.la \
	@READLINE_LIBS@

endif
