include $(top_srcdir)/common.am
lashincludedir = $(includedir)/lash-1.0/lash

lashinclude_HEADERS = \
	lash.h \
	types.h \
	config.h \
	client_interface.h \
	client_interface_new.h

if LASH_OLD_API
lashinclude_HEADERS += \
	event.h \
	protocol.h
else
noinst_HEADERS = \
	event.h \
	protocol.h
endif
