include $(top_srcdir)/common.am

lib_LTLIBRARIES = liblash.la

liblash_la_SOURCES = \
	config.c \
	comm_buffer.c \
	comm.c \
	socket.c \
	lash.c \
	event.c \
	client.c \
	comm_event.c \
	file.c \
	xmalloc.c \
	args.c \
	protocol.c \
	connect_params.c \
	loader.c \
	exec_params.c

liblash_la_CFLAGS = \
	$(LASH_CFLAGS)

liblash_la_LIBADD = \
	$(UUID_LIBS) \
	-lpthread

liblash_la_LDFLAGS = \
	-export-dynamic -version-info 2:0:0

#
# Library versions
#
# 0.3.0: 1:1:0
# 0.2: 1:0:0
# 0.1: 0:0:0
#
