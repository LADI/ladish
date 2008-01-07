AM_CXXFLAGS = @GNOMECANVASMM_CFLAGS@ @AGRAPH_CFLAGS@ @FLOWCANVAS_CFLAGS@

lib_LTLIBRARIES = libflowcanvas.la

libflowcanvas_la_LIBADD = @GNOMECANVASMM_LIBS@ @AGRAPH_LIBS@
libflowcanvas_la_LDFLAGS = -version-info @FLOWCANVAS_SO_VERSION@

libflowcanvas_la_SOURCES = \
	Canvas.cpp \
	Connectable.cpp \
	Connection.cpp \
	Ellipse.cpp \
	Item.cpp \
	Module.cpp \
	Port.cpp

