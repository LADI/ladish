SUBDIRS = lash-manual-html-one-page lash-manual-html-split

info_TEXINFOS = lash-manual.texi

EXTRA_DIST = api-proposal.html fdl.texi



if HAVE_TEXI2HTML

all-local: html-manual

html-manual: html-manual-stamp

html-manual-stamp: lash-manual.texi
	if test ! -d lash-manual-html-one-page; then \
	  mkdir lash-manual-html-one-page; \
	fi
	if test ! -d lash-manual-html-split; then \
	  mkdir -lash-manual-html-split; \
	fi
	cd lash-manual-html-one-page && \
	  texi2html -number -monolithic ../$(top_srcdir)/docs/lash-manual.texi
	cd lash-manual-html-split && \
	  texi2html -menu -number -split_node ../$(top_srcdir)/docs/lash-manual.texi
	touch html-manual-stamp

CLEANFILES = html-manual-stamp lash-manual.cps

endif # HAVE_TEXI2HTML
