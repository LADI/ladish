include $(top_srcdir)/common.am

noinst_LIBRARIES = liblashdbus.a

liblashdbus_a_SOURCES = \
	types.h \
	error.c error.h \
	signal.c signal.h \
	method.c method.h \
	interface.c interface.h \
	object_path.c object_path.h \
	introspection.c introspection.h \
	service.c service.h

# Include safemem code
nodist_liblashdbus_a_SOURCES = safety.c
BUILT_SOURCES = safety.c
CLEANFILES = safety.c
safety.c: Makefile
	cp $(top_srcdir)/common/safety.c .

liblashdbus_a_CFLAGS = \
	$(LASH_CFLAGS) \
	$(DBUS_CFLAGS)
