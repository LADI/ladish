include $(top_srcdir)/common.am

bin_PROGRAMS = lashd

lashd_SOURCES = \
	main.c \
	log.c \
	file.c file.h \
	client.c client.h \
	client_dependency.c client_dependency.h \
	loader.c loader.h \
	project.c project.h \
	store.c store.h \
	server.c server.h \
	dbus_iface_server.c dbus_iface_server.h \
	dbus_iface_control.c dbus_iface_control.h \
	dbus_service.c dbus_service.h \
	jack_patch.c jack_patch.h \
	jack_mgr_client.c jack_mgr_client.h \
	appdb.c appdb.h \
	sigsegv.c sigsegv.h \
	proctitle.c proctitle.h \
	procfs.c procfs.h \
	types.h \
	$(top_srcdir)/common/safety.c

lashd_SOURCES += \
	jackdbus_mgr.c jackdbus_mgr.h

lashd_LDADD = \
	$(XML2_LIBS) \
	$(UUID_LIBS) \
	$(DBUS_LIBS) \
	$(top_builddir)/dbus/liblashdbus.a \
	-lstdc++ -lutil

AM_CFLAGS = \
	$(LASH_CFLAGS) \
	$(XML2_CFLAGS) \
	$(DBUS_CFLAGS) \
	-DDTDDIR=\"$(dtddir)\"

# Dbus service file
servicedir = $(DBUS_SERVICES_DIR)
service_in_files = org.nongnu.lash.service.in
service_DATA = $(service_in_files:.service.in=.service)

EXTRA_DIST = $(service_in_files)
CLEANFILES = $(service_DATA)

# Rule to make the service file with bindir expanded
$(service_DATA): $(service_in_files) Makefile
	@sed -e "s|\@bindir\@|$(bindir)|" $< > $@
