raulincludedir = $(includedir)/raul

raulinclude_HEADERS = \
	Array.h \
	Atom.h \
	AtomicInt.h \
	AtomicPtr.h \
	Deletable.h \
	DoubleBuffer.h \
	JackDriver.h \
	List.h \
	MIDIRingBuffer.h \
	MIDISink.h \
	Maid.h \
	Namespaces.h \
	Path.h \
	Process.h \
	Quantizer.h \
	RDFModel.h \
	RDFNode.h \
	RDFQuery.h \
	RDFWorld.h \
	RingBuffer.h \
	SMFReader.h \
	SMFWriter.h \
	SRMWQueue.h \
	SRSWQueue.h \
	Semaphore.h \
	SharedPtr.h \
	Slave.h \
	Stateful.h \
	Thread.h \
	TimeSlice.h \
	WeakPtr.h \
	midi_events.h \
	types.h

if WITH_LIBLO
raulinclude_HEADERS += AtomLiblo.h
endif

if WITH_REDLAND
raulinclude_HEADERS += AtomRedland.h
endif

if WITH_LASH
raulinclude_HEADERS += LashClient.h LashServerInterface.h LashProject.h
endif
