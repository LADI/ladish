if BUILD_UNIT_TESTS

AM_CXXFLAGS = -I.. @REDLAND_CFLAGS@ @GLIBMM_CFLAGS@ @GTHREAD_CFLAGS@ # -DTABLE_SORT_DEBUG
ALL_LIBS = @REDLAND_LIBS@ @GLIBMM_LIBS@ @GTHREAD_LIBS@ ../src/libraul.la

noinst_PROGRAMS = \
	path_test \
	thread_test \
	queue_test \
	ringbuffer_test \
	midi_ringbuffer_test \
	atomic_test \
	list_test \
	time_test \
	quantize_test \
	smf_test \
	rdf_test \
	table_test

thread_test_LDADD = $(ALL_LIBS)
path_test_LDADD = $(ALL_LIBS)
queue_test_LDADD = $(ALL_LIBS)
ringbuffer_test_LDADD = $(ALL_LIBS)
midi_ringbuffer_test_LDADD = $(ALL_LIBS)
atomic_test_LDADD = $(ALL_LIBS)
list_test_LDADD = $(ALL_LIBS)
time_test_LDADD = $(ALL_LIBS)
quantize_test_LDADD = $(ALL_LIBS)
smf_test_LDADD = $(ALL_LIBS)
rdf_test_LDADD = $(ALL_LIBS)
table_test_LDADD = $(ALL_LIBS)

path_test_SOURCES = path_test.cpp
thread_test_SOURCES = thread_test.cpp
queue_test_SOURCES = queue_test.cpp
midi_ringbuffer_test_SOURCES = midi_ringbuffer_test.cpp
ringbuffer_test_SOURCES = ringbuffer_test.cpp
atomic_test_SOURCES = atomic_test.cpp
list_test_SOURCES = list_test.cpp
time_test_SOURCES = time_test.cpp
quantize_test_SOURCES = quantize_test.cpp
smf_test_SOURCES = smf_test.cpp
rdf_test_SOURCES = rdf_test.cpp
table_test_SOURCES = table_test.cpp

endif
