SUBDIRS = machina

lib_LTLIBRARIES = libmachina.la

libmachina_la_CXXFLAGS = @RAUL_CFLAGS@ @JACK_CFLAGS@ @GLIBMM_CFLAGS@
libmachina_la_LIBADD   = @RAUL_LIBS@   @JACK_LIBS@   @GLIBMM_LIBS@


libmachina_la_SOURCES = \
	Node.cpp \
	Machine.cpp \
	Loader.h \
	Loader.cpp \
	JackDriver.h \
	JackDriver.cpp \
	JackActions.cpp \
	JackNodeFactory.cpp
