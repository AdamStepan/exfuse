.PHONY: exfuse

exfuse::
	$(MAKE) --silent --directory src

clean::
	$(MAKE) --silent --directory src clean
	$(MAKE) --silent --directory test clean
	$(RM) -rf mp

test::
	$(MAKE) --silent --directory test
	$(MAKE) --silent --directory test test

mount: exfuse
	if [ ! -d mp ]; then\
		mkdir mp;\
	fi
	./exfuse -f mp

umount:
	fusermount -u mp
