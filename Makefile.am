SUBDIRS = redlandmm raul flowcanvas patchage slv2 omins lv2 ingen machina

distclean-local:
	rm -rf doc

docs:
	cd flowcanvas/doc && make docs
	cd raul/doc && make docs
	cd redlandmm/doc && make docs
	cd slv2/doc && make docs

upload-docs:
	echo "If you're not me, this isn't going to work..."
	scp -pr doc/flowcanvas/html/* drobilla@drobilla.net:~/drobilla.net/docs/flowcanvas
	scp -pr doc/raul/html/* drobilla@drobilla.net:~/drobilla.net/docs/raul
	scp -pr doc/redlandmm/html/* drobilla@drobilla.net:~/drobilla.net/docs/redlandmm
	scp -pr doc/slv2/html/* drobilla@drobilla.net:~/drobilla.net/docs/slv2
