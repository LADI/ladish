libraul_la_CXXFLAGS = @RAUL_CFLAGS@ @GLIBMM_CFLAGS@ @GTHREAD_CFLAGS@ @JACK_CFLAGS@
libraul_la_LIBADD = @GLIBMM_LIBS@ @GTHREAD_LIBS@ @JACK_LIBS@
libraul_la_LDFLAGS = -version-info @RAUL_SO_VERSION@

lib_LTLIBRARIES = libraul.la

libraul_la_SOURCES = \
	Maid.cpp \
	Path.cpp \
	SMFReader.cpp \
	SMFWriter.cpp \
	Symbol.cpp \
	Thread.cpp

if WITH_JACK
libraul_la_SOURCES += JackDriver.cpp
endif

