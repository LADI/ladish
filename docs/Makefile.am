SUBDIRS = lash-manual-html-one-page lash-manual-html-split

#info_TEXINFOS = lash-manual.texi

EXTRA_DIST = api-proposal.html fdl.texi lash-manual.texi.in

if HAVE_TEXI2HTML

all-local: html-manual

html-manual: html-manual-stamp

html-manual-stamp: lash-manual.texi.in
	if test ! -d lash-manual-html-one-page; then \
	  mkdir lash-manual-html-one-page; \
	fi
	if test ! -d lash-manual-html-split; then \
	  mkdir -lash-manual-html-split; \
	fi
	cd lash-manual-html-one-page && \
	  texi2html --number ../$(top_srcdir)/docs/lash-manual.texi
	cd lash-manual-html-split && \
	  texi2html --number --split=chapter ../$(top_srcdir)/docs/lash-manual.texi
	touch html-manual-stamp

CLEANFILES = html-manual-stamp lash-manual.cps

endif # HAVE_TEXI2HTML
