SUBDIRS = redlandmm raul flowcanvas patchage slv2 omins lv2 ingen machina lv2core

distclean-local:
	rm -rf doc

docs:
	cd flowcanvas/doc && make docs
	cd raul/doc && make docs
	cd redlandmm/doc && make docs
	cd slv2/doc && make docs

upload-docs:
	@echo "If you're not me, this isn't going to work..."
	@echo -e "\n\n*** Uploading flowcanvas documentation\n"
	scp -pr flowcanvas/doc/html/* drobilla@drobilla.net:~/drobilla.net/docs/flowcanvas
	@echo -e "\n\n*** Uploading raul documentation\n"
	scp -pr raul/doc/html/* drobilla@drobilla.net:~/drobilla.net/docs/raul
	@echo -e "\n\n*** Uploading redlandmm documentation\n"
	scp -pr redlandmm/doc/html/* drobilla@drobilla.net:~/drobilla.net/docs/redlandmm
	@echo -e "\n\n*** Uploading slv2 documentation\n"
	scp -pr slv2/doc/html/* drobilla@drobilla.net:~/drobilla.net/docs/slv2
