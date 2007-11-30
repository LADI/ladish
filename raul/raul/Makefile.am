raulincludedir = $(includedir)/raul

raulinclude_HEADERS = \
	Array.hpp \
	Atom.hpp \
	AtomicInt.hpp \
	AtomicPtr.hpp \
	Deletable.hpp \
	DoubleBuffer.hpp \
	JackDriver.hpp \
	List.hpp \
	ListImpl.hpp \
	MIDISink.hpp \
	Maid.hpp \
	Path.hpp \
	PathTable.hpp \
	Process.hpp \
	Quantizer.hpp \
	RingBuffer.hpp \
	SMFReader.hpp \
	SMFWriter.hpp \
	SRMWQueue.hpp \
	SRSWQueue.hpp \
	Semaphore.hpp \
	SharedPtr.hpp \
	Slave.hpp \
	StampedChunkRingBuffer.hpp \
	Stateful.hpp \
	Table.hpp \
	TableImpl.hpp \
	Thread.hpp \
	TimeSlice.hpp \
	WeakPtr.hpp \
	midi_events.h \
	types.hpp

if WITH_LIBLO
raulinclude_HEADERS += AtomLiblo.hpp
endif

if WITH_LASH
raulinclude_HEADERS += LashClient.hpp LashServerInterface.hpp LashProject.hpp
endif
