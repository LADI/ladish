all: src/Sequence.o src/ControlSet.o src/Control.o src/ControlList.o src/Note.o src/Event.o
	g++ `pkg-config --cflags --libs gtkmm-2.4 gthread-2.0` -I. test/sequence.cpp $^ -o test/sequence

src/%.o: src/%.cpp
	g++ `pkg-config --cflags gtkmm-2.4 gthread-2.0` -I. -c $^ -o $@

clean:
	rm src/*.o test/sequence
