#globalpixmapsdir = $(datadir)/pixmaps
#dist_globalpixmaps_DATA = machina-icon.svg


bin_PROGRAMS = machina

machina_CXXFLAGS = @RAUL_CFLAGS@ @JACK_CFLAGS@ @GLIBMM_CFLAGS@
machina_LDADD    = @RAUL_LIBS@   @JACK_LIBS@   @GLIBMM_LIBS@

machina_SOURCES = \
	types.hpp \
	Message.hpp \
	Node.hpp \
	Node.cpp \
	Edge.hpp \
	Machine.hpp \
	Machine.cpp \
	Loader.h \
	Loader.cpp \
	JackDriver.h \
	JackDriver.cpp \
	JackActions.hpp \
	JackActions.cpp \
	NodeFactory.hpp \
	JackNodeFactory.hpp \
	JackNodeFactory.cpp \
	main.cpp
