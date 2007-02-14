raulincludedir = $(includedir)/raul

raulinclude_HEADERS = \
	SharedPtr.h \
	WeakPtr.h \
	Path.h \
	SRSWQueue.h \
	SRMWQueue.h \
	Semaphore.h \
	Mutex.h \
	Condition.h \
	Process.h \
	Thread.h \
	Slave.h \
	Atom.h \
	JackDriver.h \
	RDFQuery.h \
	Namespaces.h \
	AtomicInt.h \
	Deletable.h \
	List.h \
	Array.h \
	Maid.h\
	Stateful.h

if WITH_LIBLO
raulinclude_HEADERS += AtomLiblo.h
endif

if WITH_RAPTOR
raulinclude_HEADERS += AtomRaptor.h
endif
