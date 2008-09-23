include $(top_srcdir)/common.am

lib_LTLIBRARIES = liblash.la

liblash_la_SOURCES = \
	lash_config.c \
	lash.c \
	client.c \
	dbus_service.c \
	dbus_iface_client.c \
	../lashd/file.c

if LASH_OLD_API
liblash_la_SOURCES += \
	protocol.c \
	event.c \
	args.c
endif

# Include code from other subdirs
nodist_liblash_la_SOURCES = \
	safety.c \
	error.c \
	method.c \
	signal.c \
	interface.c \
	object_path.c \
	introspection.c \
	service.c

BUILT_SOURCES = $(nodist_liblash_la_SOURCES)
CLEANFILES = $(nodist_liblash_la_SOURCES)

safety.c: Makefile
	cp $(top_srcdir)/common/safety.c .

error.c: Makefile
	cp $(top_srcdir)/dbus/error.c .

method.c: Makefile
	cp $(top_srcdir)/dbus/method.c .

signal.c: Makefile
	cp $(top_srcdir)/dbus/signal.c .

interface.c: Makefile
	cp $(top_srcdir)/dbus/interface.c .

object_path.c: Makefile
	cp $(top_srcdir)/dbus/object_path.c .

introspection.c: Makefile
	cp $(top_srcdir)/dbus/introspection.c .

service.c: Makefile
	cp $(top_srcdir)/dbus/service.c .

liblash_la_CFLAGS = \
	$(LASH_CFLAGS) \
	$(DBUS_CFLAGS) \
	-DDEBUG_OUTPUT_TERMINAL

liblash_la_LIBADD = \
	$(UUID_LIBS) \
	$(DBUS_LIBS)

liblash_la_LDFLAGS = \
	-export-dynamic -version-info 2:1:1

# Be sure to read before updating version info:
#   http://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html

#
# Released library versions
#
# LADCCA: < 2:0:0
# 0.5.0: 2:0:0
# 0.5.1: 2:1:0
# 0.5.2: 2:0:1   <-
# 0.5.3: 2:1:1   <- (Version info updated incorrectly)
# 0.5.4: 2:1:1   <-

#
# Planned library versions
#
# 0.5.5: 2:2:1 (source changes)
# 0.6.0: 3:0:2 (new interfaces)
# 0.6.1: 3:1:2 (source changes)
