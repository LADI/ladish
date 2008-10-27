CFLAGS = -O0 -g -ansi -Wall -Wextra -Wno-unused # -pedantic -Woverloaded-virtual
CFLAGS += -fPIC -DPIC -Ilvz -Ivstgui -I. -DURI_PREFIX=\"http://drobilla.net/ns/dev/mda-lv2/\"

SYSTEMNAME = $(shell uname -s)

ifeq ($(SYSTEMNAME),Darwin)
CFLAGS += -fno-common -flat_namespace 
SHARED_LDFLAGS = -fno-common -flat_namespace -bundle -undefined suppress -lbundle1.o -nostartfiles
USER_INSTALL_DIR = ~/Library/Audio/Plug-Ins/LV2/
LOCAL_INSTALL_DIR = /Library/Audio/Plug-Ins/LV2/
SYSTEM_INSTALL_DIR = /Library/Audio/Plug-Ins/LV2/
else
SHARED_LDFLAGS = -shared
USER_INSTALL_DIR = ~/.lv2/
SYSTEM_INSTALL_DIR = /usr/lib/lv2/
LOCAL_INSTALL_DIR = /usr/local/lib/lv2/
endif

BUILD_GUI = ! `pkg-config --exists gtk+-2.0`
GUI_CFLAGS = $(CFLAGS) -Ivstgui `pkg-config --cflags gtk+-2.0 libpng`

all: lvz/gendata libs data gui_libs

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
	mda.lv2/mdaVocoder.so \
	mda.lv2/mdaSpecMeter.so

pixmaps:
	cp src/mdaSpecMeter.png mda.lv2

gui_libs: bundle pixmaps \
	mda.lv2/mdaSpecMeterGUI.so

data: libs gui_libs lvz/gendata
	cd ./mda.lv2 && ../lvz/gendata ./*.so > manifest.ttl
	@echo "*** Ignore the above non-error about loading images! ***"

install:
	if [ "x$(INSTALL_DIR)" = "x" ]; then \
		echo -e "\n*** ERROR: INSTALL_DIR is not set\n"; \
		echo -e "Try make install-user, install-local, or install-system\n"; \
		echo -e "You can also specify where to install the plugin bundle:"; \
		echo -e "    make install INSTALL_DIR=~/.lv2/\n"; \
	else \
		install -d $(INSTALL_DIR)/mda.lv2; \
		install -m 644 ./mda.lv2/*.ttl $(INSTALL_DIR)/mda.lv2; \
		install -m 644 ./mda.lv2/*.png $(INSTALL_DIR)/mda.lv2; \
		install -m 755 ./mda.lv2/*.so $(INSTALL_DIR)/mda.lv2; \
	fi

install-user:
	INSTALL_DIR=$(USER_INSTALL_DIR) make install

install-local:
	INSTALL_DIR=$(LOCAL_INSTALL_DIR) make install

install-system:
	INSTALL_DIR=$(SYSTEM_INSTALL_DIR) make install

uninstall:
	rm -rf $(HOME)/.lv2/mda.lv2
	rm -rf /usr/local/lib/lv2/mda.lv2
	rm -rf /usr/lib/lv2/mda.lv2

src/%.cpp: src/%.h lvz/audioeffectx.h

lvz/gendata: lvz/gendata.cpp lvz/audioeffectx.h
	$(CXX) $(CFLAGS) -ldl $< -o $@

mda.lv2/%GUI.so: src/%GUI.cpp src/%.cpp lvz/gui_wrapper.cpp vstgui/vstgui.cpp vstgui/vstgui.h vstgui/vstcontrols.cpp vstgui/vstcontrols.h
	if [ $(BUILD_GUI) ]; then \
	$(CXX) $(SHARED_LDFLAGS) $(GUI_CFLAGS) \
		-DUI_CLASS=`echo $@ | sed 's/mda.lv2\///' | sed 's/\..*//'` \
		-DPLUGIN_CLASS=`echo $@ | sed 's/mda.lv2\///' | sed 's/\..*//' | sed 's/GUI//'` \
		-DUI_HEADER=\"`echo $@ | sed 's/^mda.lv2/src/' | sed 's/\(.*\)\..*/\1/' | sed 's/$$/\.h/'`\" \
		-DPLUGIN_HEADER=\"`echo $@ | sed 's/^mda.lv2/src/' | sed 's/\(.*\)\..*/\1/' | sed 's/$$/\.h/' | sed 's/GUI//'`\" \
		-DUI_URI_SUFFIX=\"`echo $@ | sed 's/mda.lv2\///' | sed 's/^mda//' | sed 's/\..*//'`\" \
		-DPLUGIN_URI_SUFFIX=\"`echo $@ | sed 's/mda.lv2\///' | sed 's/^mda//' | sed 's/\..*//' | sed 's/GUI//'`\" \
		$^ -o $@; \
	fi

mda.lv2/%.so: src/%.cpp lvz/wrapper.cpp
	$(CXX) $(SHARED_LDFLAGS) $(CFLAGS) \
		-DPLUGIN_CLASS=`echo $@ | sed 's/mda.lv2\///' | sed 's/\..*//'` \
		-DPLUGIN_URI_SUFFIX=\"`echo $@ | sed 's/mda.lv2\///' | sed 's/^mda//' | sed 's/\..*//'`\" \
		-DPLUGIN_HEADER=\"`echo $@ | sed 's/^mda.lv2/src/' | sed 's/\(.*\)\..*/\1/' | sed 's/$$/\.h/'`\" \
		$^ -o $@

clean:
	rm -f `find -name '*.o'`
	rm -f `find -name '*.so'`
	rm -f `find -name '*.ttl'`
	rm -f lvz/gendata
	rm -rf ./mda.lv2

