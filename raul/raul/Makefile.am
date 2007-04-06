raulincludedir = $(includedir)/raul

raulinclude_HEADERS = \
	types.h \
	midi_events.h \
	Array.h \
	Atom.h \
	AtomLiblo.h \
	AtomRaptor.h \
	AtomicInt.h \
	AtomicPtr.h \
	DoubleBuffer.h \
	Condition.h \
	Deletable.h \
	JackDriver.h \
	List.h \
	Maid.h \
	Mutex.h \
	Namespaces.h \
	Path.h \
	Process.h \
	RDFQuery.h \
	RDFWriter.h \
	SRMWQueue.h \
	SRSWQueue.h \
	RingBuffer.h \
	Semaphore.h \
	SharedPtr.h \
	Slave.h \
	Stateful.h \
	Thread.h \
	WeakPtr.h \
	TimeSlice.h \
	Quantizer.h \
	MIDIRingBuffer.h \
	MIDISink.h \
	SMFReader.h \
	SMFWriter.h

if WITH_LIBLO
raulinclude_HEADERS += AtomLiblo.h
endif

if WITH_RAPTOR
raulinclude_HEADERS += AtomRaptor.h
endif

if WITH_LASH
raulinclude_HEADERS += LashClient.h LashServerInterface.h LashProject.h
endif
