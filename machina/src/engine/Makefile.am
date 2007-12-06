SUBDIRS = machina

lib_LTLIBRARIES = libmachina.la

libmachina_la_CXXFLAGS = @REDLANDMM_CFLAGS@ @RAUL_CFLAGS@ @JACK_CFLAGS@ @GLIBMM_CFLAGS@
libmachina_la_LIBADD   = @REDLANDMM_LIBS@ @RAUL_LIBS@ @JACK_LIBS@   @GLIBMM_LIBS@

libmachina_la_SOURCES = \
	Action.cpp \
	ActionFactory.cpp \
	Edge.cpp \
	Engine.cpp \
	LearnRequest.cpp \
	Loader.cpp \
	Machine.cpp \
	MachineBuilder.cpp \
	MidiAction.cpp \
	Mutation.cpp \
	Node.cpp \
	Problem.cpp \
	Recorder.cpp \
	SMFDriver.cpp
	
if WITH_JACK
libmachina_la_SOURCES += JackDriver.cpp jack_compat.h
endif
