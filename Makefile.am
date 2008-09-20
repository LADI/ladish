SUBDIRS = m4 docs lash common dbus liblash lashd clients icons pylash

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

pacoinstall: svnversion.h # SVNONLY
	test `whoami` == root # test for root SVNONLY
	-paco -rvB lash # SVNONLY
	-paco -lE `pwd` -p lash-$(shell $(top_srcdir)/svnversion.sh $(top_srcdir)) "make install" # SVNONLY
	-paco -i lash # SVNONLY

svnversion.h:: # SVNONLY
	@if test -d $(top_srcdir)/.svn; then $(top_srcdir)/svnversion_regenerate.sh $@; fi # SVNONLY

DISTCLEANFILES = svnversion.h # SVNONLY
BUILT_SOURCES = svnversion.h # SVNONLY
EXTRA_DIST += svnversion.h svnversion.sh

dist-hook: # SVNONLY
	sed -i 's/^.*SVNONLY.*//' $(distdir)/Makefile.am $(distdir)/Makefile.in # SVNONLY

# temporary hack for preview tarballs
# should be disabled before releasing official tarball
SVN_VERSION = $(shell $(top_srcdir)/svnversion.sh $(top_srcdir))
distdir = $(PACKAGE)-$(VERSION).$(SVN_VERSION)
