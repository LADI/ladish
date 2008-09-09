if BUILD_UNIT_TESTS

AM_CXXFLAGS = -I.. @REDLAND_CFLAGS@ @GLIBMM_CFLAGS@ @GTHREAD_CFLAGS@
ALL_LIBS = @REDLAND_LIBS@ @GLIBMM_LIBS@ @GTHREAD_LIBS@ ../src/libredlandmm.la

noinst_PROGRAMS = \
	rdf_test

rdf_test_LDADD = $(ALL_LIBS)

rdf_test_SOURCES = rdf_test.cpp

endif
