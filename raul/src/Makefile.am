libraul_la_CXXFLAGS = -I$(top_srcdir) @REDLAND_CFLAGS@ @GLIBMM_CFLAGS@ @JACK_CFLAGS@ @LASH_CFLAGS@
libraul_la_LIBADD = @REDLAND_LIBS@ @GLIBMM_LIBS@ @JACK_LIBS@ @LASH_LIBS@

lib_LTLIBRARIES = libraul.la

libraul_la_SOURCES = \
	Maid.cpp \
	Namespaces.cpp \
	Path.cpp \
	RDFWorld.cpp \
	SMFReader.cpp \
	SMFWriter.cpp \
	Thread.cpp

if WITH_REDLAND
libraul_la_SOURCES += RDFModel.cpp RDFNode.cpp RDFQuery.cpp
endif

if WITH_JACK
libraul_la_SOURCES += JackDriver.cpp
endif

if WITH_LASH
libraul_la_SOURCES += LashClient.cpp LashServerInterface.cpp LashProject.cpp
endif
