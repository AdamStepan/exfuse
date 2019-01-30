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
	if [ ! -f exdev ]; then \
		./exmkfs --create --device exdev --inodes 8; \
	fi
	gdb --args ./exfuse -f mp --device exdev

mount: exfuse umount | dir
	if [ ! -f exdev ]; then \
		./exmkfs --create --device exdev; \
	fi
	./exfuse -f mp --device exdev

umount:
	fusermount -u mp || true
