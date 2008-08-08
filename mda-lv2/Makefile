CFLAGS = -Wall -fPIC -Ilvz -DPLUGIN_URI=\"http://example.org/mda-plugin\"

all: \
	src/mdaAmbience.so \
	src/mdaBandisto.so \
	src/mdaBeatBox.so \
	src/mdaCombo.so \
	src/mdaDX10.so \
	src/mdaDeEss.so \
	src/mdaDegrade.so \
	src/mdaDelay.so \
	src/mdaDetune.so \
	src/mdaDither.so \
	src/mdaDubDelay.so \
	src/mdaDynamics.so \
	src/mdaEPiano.so \
	src/mdaImage.so \
	src/mdaJX10.so \
	src/mdaLeslie.so \
	src/mdaLimiter.so \
	src/mdaLoudness.so \
	src/mdaMultiBand.so \
	src/mdaOverdrive.so \
	src/mdaPiano.so \
	src/mdaRePsycho.so \
	src/mdaRezFilter.so \
	src/mdaRingMod.so \
	src/mdaRoundPan.so \
	src/mdaShepard.so \
	src/mdaSplitter.so \
	src/mdaStereo.so \
	src/mdaSubSynth.so \
	src/mdaTalkBox.so \
	src/mdaTestTone.so \
	src/mdaThruZero.so \
	src/mdaTracker.so \
	src/mdaTransient.so \
	src/mdaVocInput.so \
	src/mdaVocoder.so

src/%.so: src/%.cpp lvz/wrapper.cpp
	$(CC) -shared $(CFLAGS) -DPLUGIN_CLASS=`echo $@ | sed 's/\..*//' | sed 's/src\///'` \
		-DPLUGIN_HEADER=\"`echo $@ | sed 's/\..*//' | sed 's/src\//\.\.\/src\//' | sed 's/$$/\.h/'`\" \
		$< lvz/wrapper.cpp -o $@

clean:
	rm -f src/*.o src/*.so


