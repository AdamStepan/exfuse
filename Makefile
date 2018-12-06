.PHONY: exfuse

exfuse::
	$(MAKE) --directory src

clean:: umount
	$(MAKE) --directory src clean
	$(MAKE) --directory test clean
	$(RM) -rf mp exdev
	find . -type f -name core -exec rm {} ';'

test::
	$(MAKE) --directory test
	$(MAKE) --directory test test

dir::
	if [ ! -d mp ]; then\
		mkdir mp;\
	fi

debug:: exfuse umount | dir
	gdb --args ./exfuse -f mp

mount: exfuse umount | dir
	./exfuse -f mp

umount:
	fusermount -u mp || true
