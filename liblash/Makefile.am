include $(top_srcdir)/common.am

lib_LTLIBRARIES = liblash.la

liblash_la_SOURCES  = lash.c
liblash_la_SOURCES += lash_config.c
liblash_la_SOURCES += client.c
liblash_la_SOURCES += dbus_service.c
liblash_la_SOURCES += dbus_iface_client.c

liblash_la_SOURCES += $(top_srcdir)/common/safety.c
liblash_la_SOURCES += $(top_srcdir)/dbus/error.c
liblash_la_SOURCES += $(top_srcdir)/dbus/method.c
liblash_la_SOURCES += $(top_srcdir)/dbus/signal.c
liblash_la_SOURCES += $(top_srcdir)/dbus/interface.c
liblash_la_SOURCES += $(top_srcdir)/dbus/object_path.c
liblash_la_SOURCES += $(top_srcdir)/dbus/introspection.c
liblash_la_SOURCES += $(top_srcdir)/dbus/service.c
liblash_la_SOURCES += $(top_srcdir)/lashd/file.c

if LASH_OLD_API
liblash_la_SOURCES += \
	protocol.c \
	event.c \
	args.c
endif

AM_CFLAGS = $(LASH_CFLAGS) $(DBUS_CFLAGS) -DDEBUG_OUTPUT_TERMINAL

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
