.PHONY: exfuse

exfuse::
	$(MAKE) --directory src

clean::
	$(MAKE) --directory src clean
	$(MAKE) --directory test clean
	$(RM) -rf mp exdev

test::
	$(MAKE) --directory test
	$(MAKE) --directory test test

mount: exfuse
	if [ ! -d mp ]; then\
		mkdir mp;\
	fi
	./exfuse -f mp

umount:
	fusermount -u mp
