
ORIGDIRS = $(wildcard *.orig)
CVSDIRS = $(wildcard *.cvs)
ALLDIRS = $(ORIGDIRS) $(CVSDIRS)

DIFFS = $(ORIGDIRS:%.orig=%.diff)
CVSDIFFS = $(CVSDIRS:%.cvs=%.cvs-diff)

DIFFPATCHES = $(DIFFS:%=%.gz)
CVSDIFFPATCHES = $(CVSDIFFS:%=%.gz)
PATCHES = $(DIFFPATCHES) $(CVSDIFFPATCHES)

all-local: patches-stamp

diffs-stamp: $(DIFFS) $(DIFFPATCHES)
	touch diffs-stamp

cvsdiffs-stamp: $(CVSDIFFS) $(CVSDIFFPATCHES)
	touch cvsdiffs-stamp

patches-stamp: diffs-stamp cvsdiffs-stamp
	touch patches-stamp

find_dev_files = \
  $(shell \
      find $(1) -type f \
        -name "*.h" -or \
        -name "*.c" -or \
        -name "*.cc" -or \
        -name "*.cxx" -or \
        -name "*.cpp" -or \
        -name "*.CC" -or \
        -name "Makefile*" -or \
        -name "configure*"; )


$(CVSDIFFS): %.cvs-diff: $(foreach dir,$(CVSDIRS),$(call find_dev_files,$(dir)))
	cd $*.cvs && ( cvs diff > ../$@ || true )


$(DIFFS): %.diff: $(foreach dir,$(ORIGDIRS:%.orig=%),$(call find_dev_files,$(dir)))
	diff -r -u $*.orig $* > $@ || true


%.gz: %
	gzip -c --best -f $< > $@

CLEANFILES = $(PATCHES) $(DIFFS) $(CVSDIFFS) patches-stamp diffs-stamp cvsdiffs-stamp
EXTRA_DIST = $(PATCHES)

cleansources:
	for dir in $(ORIGDIRS:%.orig=%); do \
	  make -C $$dir clean || true; \
	  make -C $$dir distclean || true; \
	done
