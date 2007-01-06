if BUILD_TESTS

AM_CXXFLAGS = -I.. -lpthread
bin_PROGRAMS = path_test thread_test queue_test

thread_test_LDADD = ../src/libraul.la

path_test_SOURCES = path_test.cpp
thread_test_SOURCES = thread_test.cpp
queue_test_SOURCES = queue_test.cpp

endif
