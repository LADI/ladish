
#gcc `pkg-config --cflags --libs gtkmm-2.4` -o roll sequence.cpp seqkeys.cpp seqtime.cpp seqdata.cpp seqevent.cpp seqedit.cpp main.cpp
CXXFLAGS=`pkg-config --cflags gtkmm-2.4`
LDFLAGS=`pkg-config --libs gtkmm-2.4`
all: roll

OBJECTS = event.o sequence.o seqkeys.o seqtime.o seqdata.o seqevent.o seqedit.o seqroll.o main.o

roll: $(OBJECTS)
	g++ $(LDFLAGS) -o roll $(OBJECTS)

clean:
	rm $(OBJECTS)
