CFLAGS = -O0 -g -Wall -fPIC -Ilvz -DPLUGIN_URI_PREFIX=\"http://drobilla.net/ns/plugins/mda-lv2/\"

all: lvz/gendata libs data

libs: \
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

data: libs
	lvz/gendata src/*.so

src/%.c: src/%.h lvz/audioeffectx.h

lvz/gendata: lvz/gendata.cpp lvz/audioeffectx.h
	$(CXX) $(CFLAGS) -ldl $< -o $@

src/%.so: src/%.cpp lvz/wrapper.cpp
	basename="foo"
	$(CXX) -shared $(CFLAGS) \
		-DPLUGIN_CLASS=`echo $@ | sed 's/\..*//' | sed 's/src\///'` \
		-DPLUGIN_URI_SUFFIX=\"`echo $@ | sed 's/\..*//' | sed 's/src\///' | sed 's/^mda//'`\" \
		-DPLUGIN_HEADER=\"`echo $@ | sed 's/\..*//' | sed 's/src\//\.\.\/src\//' | sed 's/$$/\.h/'`\" \
		$< lvz/wrapper.cpp -o $@

clean:
	rm -f src/*.o src/*.so lvz/gendata src/*.ttl


