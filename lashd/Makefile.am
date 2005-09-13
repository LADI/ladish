include $(top_srcdir)/common.am

bin_PROGRAMS = lashd

lashd_SOURCES = \
	main.c \
	conn.c conn.h \
	client.c client.h \
	conn_mgr.c conn_mgr.h \
	server_event.c server_event.h \
	project.c project.h \
	store.c store.h \
	server.c server.h \
	client_event.c client_event.h \
	alsa_patch.c alsa_patch.h \
	jack_patch.c jack_patch.h \
	jack_mgr.c jack_mgr.h \
	jack_mgr_client.c jack_mgr_client.h \
	jack_fport.c jack_fport.h \
	alsa_fport.c alsa_fport.h \
	alsa_mgr.c alsa_mgr.h \
	alsa_client.c alsa_client.h \
	globals.h

lashd_LDADD = \
	$(LASH_LIBS) \
	$(JACK_LIBS) \
	$(ALSA_LIBS) \
	$(XML2_LIBS) \
	$(UUID_LIBS) \
        $(top_builddir)/liblash/liblash.la

lashd_CFLAGS = \
	$(LASH_CFLAGS) \
	$(JACK_CFLAGS) \
	$(ALSA_CFLAGS) \
	$(XML2_CFLAGS) \
	-DDTDDIR=\"$(dtddir)\"

dtddir = $(pkgdatadir)/dtds
dtd_DATA = lash-project-1.0.dtd

EXTRA_DIST = $(dtd_DATA)

if SERVICE_INSTALL
install-data-local:
	if ! grep -q ^lash /etc/services; then \
	  echo -e "\nlash\t\t14541/tcp\t\t\t# LASH client/server protocol" >> /etc/services; \
	fi
endif
