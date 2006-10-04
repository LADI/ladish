raulincludedir = $(includedir)/raul

raulinclude_HEADERS = \
	SharedPtr.h \
	WeakPtr.h \
	Path.h \
	Queue.h \
	Semaphore.h \
	Mutex.h \
	Condition.h \
	Thread.h \
	Slave.h \
	Atom.h

if WITH_LIBLO
raulinclude_HEADERS += AtomLiblo.h
endif

if WITH_RAPTOR
raulinclude_HEADERS += AtomRaptor.h
endif
