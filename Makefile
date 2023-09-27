

all:
	$(MAKE) -C cpu/src
	./cpu/src/vcpu_scheduler 2