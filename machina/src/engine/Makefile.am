SUBDIRS = machina

lib_LTLIBRARIES = libmachina.la

libmachina_la_CXXFLAGS = @RAUL_CFLAGS@ @JACK_CFLAGS@ @GLIBMM_CFLAGS@
libmachina_la_LIBADD   = @RAUL_LIBS@   @JACK_LIBS@   @GLIBMM_LIBS@

libmachina_la_SOURCES = \
	Node.cpp \
	Edge.cpp \
	Action.cpp \
	Machine.cpp \
	Loader.cpp \
	MidiAction.cpp \
	SMFDriver.cpp \
	Engine.cpp \
	LearnRequest.cpp
	
if WITH_JACK
libmachina_la_SOURCES += JackDriver.cpp
endif
