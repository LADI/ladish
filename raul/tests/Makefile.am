if BUILD_TESTS

AM_CXXFLAGS = -I.. -lpthread @RASQAL_CFLAGS@
ALL_LIBS = @RASQAL_LIBS@ ../src/libraul.la

bin_PROGRAMS = path_test thread_test queue_test

thread_test_LDADD = $(ALL_LIBS)
path_test_LDADD = $(ALL_LIBS)
queue_test_LDADD = $(ALL_LIBS)

path_test_SOURCES = path_test.cpp
thread_test_SOURCES = thread_test.cpp
queue_test_SOURCES = queue_test.cpp

endif
