TOP:=$(shell pwd)

.PHONY: ladish
ladish:
	rm -rf jack2
	git clone -b $(shell git symbolic-ref --short HEAD) --depth=1 https://github.com/LADI/cdbus
	cd cdbus && python3 ./waf configure --prefix=$(TOP)/build/destdir/usr
	cd cdbus && python3 ./waf
	cd cdbus && python3 ./waf install
	git clone -b $(shell git symbolic-ref --short HEAD) --depth=1 --recurse-submodules --shallow-submodules https://github.com/LADI/jack2
	cd jack2 && PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:$(TOP)/build/destdir/usr/lib/pkgconfig python3 ./waf configure --prefix=$(TOP)/build/destdir/usr
	cd jack2 && python3 ./waf
	cd jack2 && python3 ./waf install
	PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:$(TOP)/build/destdir/usr/lib/pkgconfig python3 ./waf configure --prefix=$(TOP)/build/destdir/usr
	python3 ./waf
	python3 ./waf install

.PHONY: html
html: index.html LADI.html

index.html: README.adoc README-docinfo.html README-docinfo-header.html
	asciidoc -b html5 -a data-uri -a icons --theme ladi -o index.html README.adoc

LADI.html: doc/LADI.adoc README-docinfo.html README-docinfo-header.html
	asciidoc -b html5 -a data-uri -a icons --theme ladi -o LADI.html doc/LADI.adoc
