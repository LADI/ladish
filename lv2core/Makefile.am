AUTOMAKE_OPTIONS = foreign

SUBDIRS = lv2core.lv2 debian-sid

lv2includedir = $(includedir)
lv2include_HEADERS = lv2.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = lv2core.pc

deb:
	if [ -e debian ]; then \
		echo "ERROR: ./debian exists, you must move it out of the way."; \
	else \
		ln -s debian-sid debian; \
		dpkg-buildpackage -sn; \
		rm debian; \
	fi
