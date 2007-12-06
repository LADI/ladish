SUBDIRS = machina

lib_LTLIBRARIES = libmachina.la

libmachina_la_CXXFLAGS = @REDLANDMM_CFLAGS@ @RAUL_CFLAGS@ @JACK_CFLAGS@ @GLIBMM_CFLAGS@
libmachina_la_LIBADD   = @REDLANDMM_LIBS@ @RAUL_LIBS@ @JACK_LIBS@   @GLIBMM_LIBS@

libmachina_la_SOURCES = \
	Node.cpp \
	Edge.cpp \
	Action.cpp \
	Machine.cpp \
	Mutation.cpp \
	Loader.cpp \
	MidiAction.cpp \
	ActionFactory.cpp \
	SMFDriver.cpp \
	Engine.cpp \
	LearnRequest.cpp \
	Recorder.cpp \
	MachineBuilder.cpp
	
if WITH_JACK
libmachina_la_SOURCES += JackDriver.cpp jack_compat.h
endif
