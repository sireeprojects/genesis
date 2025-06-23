DBG ?= on
DEV ?= on

ifeq "$(DBG)" "on"
LIBPARAMS += -DCEA_DEBUG
else
LIBPARAMS +=
endif

ifeq "$(DEV)" "on"
LIBPARAMS += -DCEA_DEVEL
else
LIBPARAMS +=
endif

test:
	@g++ test.cpp -O3 -o sim.x -lcea -L${PWD} ${LIBPARAMS}
	@./sim.x

sim:lib
	@g++ test.cpp -O3 -o sim.x -lcea -L${PWD} ${LIBPARAMS}
	@./sim.x

lib:clean
	@g++ cea.cpp -O3 -s -fPIC -shared -o libcea.so -Wall -Wno-unused -lpthread ${LIBPARAMS}

clean:
	@rm -rf *.x *.log *.so *.o *.pcap

copy:
	@[ -f "run.pcap" ] && cp run.pcap /mnt/hgfs/shared || echo ""

