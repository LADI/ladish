#CFLAGS = -O0 -g -ansi -pedantic -Wall -Wextra -Wshadow -Woverloaded-virtual -Wno-unused
CFLAGS += -fPIC -DPIC -Ilvz -I. -DPLUGIN_URI_PREFIX=\"http://drobilla.net/ns/dev/mda-lv2/\"

all: lvz/gendata libs data

bundle:
	mkdir -p ./mda.lv2

libs: bundle \
	mda.lv2/mdaAmbience.so \
	mda.lv2/mdaBandisto.so \
	mda.lv2/mdaBeatBox.so \
	mda.lv2/mdaCombo.so \
	mda.lv2/mdaDX10.so \
	mda.lv2/mdaDeEss.so \
	mda.lv2/mdaDegrade.so \
	mda.lv2/mdaDelay.so \
	mda.lv2/mdaDetune.so \
	mda.lv2/mdaDither.so \
	mda.lv2/mdaDubDelay.so \
	mda.lv2/mdaDynamics.so \
	mda.lv2/mdaEPiano.so \
	mda.lv2/mdaImage.so \
	mda.lv2/mdaJX10.so \
	mda.lv2/mdaLeslie.so \
	mda.lv2/mdaLimiter.so \
	mda.lv2/mdaLoudness.so \
	mda.lv2/mdaMultiBand.so \
	mda.lv2/mdaOverdrive.so \
	mda.lv2/mdaPiano.so \
	mda.lv2/mdaRePsycho.so \
	mda.lv2/mdaRezFilter.so \
	mda.lv2/mdaRingMod.so \
	mda.lv2/mdaRoundPan.so \
	mda.lv2/mdaShepard.so \
	mda.lv2/mdaSplitter.so \
	mda.lv2/mdaStereo.so \
	mda.lv2/mdaSubSynth.so \
	mda.lv2/mdaTalkBox.so \
	mda.lv2/mdaTestTone.so \
	mda.lv2/mdaThruZero.so \
	mda.lv2/mdaTracker.so \
	mda.lv2/mdaTransient.so \
	mda.lv2/mdaVocInput.so \
	mda.lv2/mdaVocoder.so

data: libs lvz/gendata
	cd ./mda.lv2 && ../lvz/gendata ./*.so > manifest.ttl

install:
	if [ "x$(INSTALL_DIR)" = "x" ]; then \
		echo -e "\n*** ERROR: INSTALL_DIR is not set\n"; \
		echo -e "Try make install-user, install-local, or install-system\n"; \
		echo -e "You can also specify where to install the plugin bundle:"; \
		echo -e "    make install INSTALL_DIR=~/.lv2/\n"; \
	else \
		install -d $(INSTALL_DIR)/mda.lv2; \
		install -m 644 ./mda.lv2/*.ttl $(INSTALL_DIR)/mda.lv2; \
		install -m 755 ./mda.lv2/*.so $(INSTALL_DIR)/mda.lv2; \
	fi

install-user:
	INSTALL_DIR=$(HOME)/.lv2 make install

install-local:
	INSTALL_DIR=/usr/local/lib/lv2 make install

install-system:
	INSTALL_DIR=/usr/lib/lv2 make install

uninstall:
	rm -rf $(HOME)/.lv2/mda.lv2
	rm -rf /usr/local/lib/lv2/mda.lv2
	rm -rf /usr/lib/lv2/mda.lv2

src/%.cpp: src/%.h lvz/audioeffectx.h

lvz/gendata: lvz/gendata.cpp lvz/audioeffectx.h
	$(CXX) $(CFLAGS) -ldl $< -o $@

mda.lv2/%.so: src/%.cpp lvz/wrapper.cpp
	$(CXX) -shared $(CFLAGS) \
		-DPLUGIN_CLASS=`echo $@ | sed 's/mda.lv2\///' | sed 's/\..*//'` \
		-DPLUGIN_URI_SUFFIX=\"`echo $@ | sed 's/mda.lv2\///' | sed 's/^mda//' | sed 's/\..*//'`\" \
		-DPLUGIN_HEADER=\"`echo $@ | sed 's/^mda.lv2/src/' | sed 's/\(.*\)\..*/\1/' | sed 's/$$/\.h/'`\" \
		$< lvz/wrapper.cpp -o $@

clean:
	rm -f `find -name '*.o'`
	rm -f `find -name '*.so'`
	rm -f `find -name '*.ttl'`
	rm -f lvz/gendata
	rm -rf ./mda.lv2

