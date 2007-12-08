SUBDIRS = machina

lib_LTLIBRARIES = libmachina.la

libmachina_la_CXXFLAGS = @EUGENE_CFLAGS@ @REDLANDMM_CFLAGS@ @RAUL_CFLAGS@ @JACK_CFLAGS@ @GLIBMM_CFLAGS@
libmachina_la_LIBADD   = @EUGENE_LIBS@   @REDLANDMM_LIBS@   @RAUL_LIBS@   @JACK_LIBS@   @GLIBMM_LIBS@

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
	Recorder.cpp \
	SMFDriver.cpp
	
if WITH_JACK
libmachina_la_SOURCES += JackDriver.cpp
endif

if WITH_EUGENE
libmachina_la_SOURCES += Evolver.cpp Problem.cpp
endif
