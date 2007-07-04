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
	-export-dynamic -version-info 2:1:1

#
# Library versions
# (< 2:0:0 was the "ladcca" days)
#
# 0.5.0: 2:0:0
# 0.5.1: 2:1:0
# 0.5.2: 2:0:1
# 0.5.3: 2:1:1
