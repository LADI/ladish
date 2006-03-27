SUBDIRS = m4 docs lash liblash lashd clients icons

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = lash-1.0.pc

EXTRA_DIST = common.am README.SECURITY \
	lash-configure-template ChangeLog.old

buildcheck:
	cd $(top_srcdir) && make distclean
	cd $(top_srcdir) && ./autogen.sh --no-conf
	mkdir -p $(top_srcdir)/buildcheck
	cd $(top_srcdir)/buildcheck && \
          CFLAGS="$$CFLAGS -std=c99 -Werror -Wall -pedantic -Wno-switch" \
          ../configure \
            --disable-debug \
            --disable-gtk \
            --enable-maintainer-mode
	cd $(top_srcdir)/buildcheck && make
	cd $(top_srcdir)/buildcheck && make distclean
	rm -rf $(top_srcdir)/buildcheck
	cd $(top_srcdir) && ./autogen.sh
