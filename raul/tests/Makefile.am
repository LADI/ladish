if BUILD_TESTS

AM_CXXFLAGS = -I.. -lpthread @RASQAL_CFLAGS@ @GLIBMM_CFLAGS@
ALL_LIBS = @RASQAL_LIBS@ @GLIBMM_LIBS@ ../src/libraul.la

bin_PROGRAMS = path_test thread_test queue_test atomic_test list_test time_test	quantize_test

thread_test_LDADD = $(ALL_LIBS)
path_test_LDADD = $(ALL_LIBS)
queue_test_LDADD = $(ALL_LIBS)
atomic_test_LDADD = $(ALL_LIBS)
list_test_LDADD = $(ALL_LIBS)
time_test_LDADD = $(ALL_LIBS)
quantize_test_LDADD = $(ALL_LIBS)

path_test_SOURCES = path_test.cpp
thread_test_SOURCES = thread_test.cpp
queue_test_SOURCES = queue_test.cpp
atomic_test_SOURCES = atomic_test.cpp
list_test_SOURCES = list_test.cpp
time_test_SOURCES = time_test.cpp
quantize_test_SOURCES = quantize_test.cpp

endif
