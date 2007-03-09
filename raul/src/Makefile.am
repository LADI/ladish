libraul_la_CXXFLAGS = -I$(top_srcdir) @RAPTOR_CFLAGS@ @RASQAL_CFLAGS@ @GLIBMM_CFLAGS@ @JACK_CFLAGS@
libraul_la_LIBADD = @RAPTOR_LIBS@ @RASQAL_LIBS@ @GLIBMM_LIBS@ @JACK_LIBS@

lib_LTLIBRARIES = libraul.la

libraul_la_SOURCES = \
	Thread.cpp \
	Path.cpp \
	Namespaces.cpp \
	Maid.cpp \
	SMFReader.cpp \
	SMFWriter.cpp

if WITH_RAPTOR
libraul_la_SOURCES += RDFWriter.cpp
endif

if WITH_RASQAL
libraul_la_SOURCES += RDFQuery.cpp
endif

if WITH_JACK
libraul_la_SOURCES += JackDriver.cpp
endif
