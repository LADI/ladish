AM_CXXFLAGS = @GNOMECANVASMM_CFLAGS@ @AGRAPH_CFLAGS@ -I$(top_srcdir)/flowcanvas

lib_LTLIBRARIES = libflowcanvas.la

libflowcanvas_la_LIBADD = @GNOMECANVASMM_LIBS@ @AGRAPH_LIBS@

libflowcanvas_la_SOURCES = \
	Canvas.cpp \
	Connectable.cpp \
	Connection.cpp \
	Ellipse.cpp \
	Item.cpp \
	Module.cpp \
	Port.cpp

