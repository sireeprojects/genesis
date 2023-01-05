DBG ?= on

ifeq "$(DBG)" "on"
LIBPARAMS += -DCEA_DEBUG
else
LIBPARAMS +=
endif

sim:makelib
	@g++ test.cpp -O3 -o sim.x -lcea -L${PWD} ${LIBPARAMS}
	@./sim.x;

makelib:clean
	@g++ cea.cpp -O3 -s -fPIC -shared -o libcea.so -Wall -Wno-unused -lpthread ${LIBPARAMS}

clean:
	@rm -rf *.x *.log *.so *.o frames.pcap

copy:
	@[ -f "run.pcap" ] && cp run.pcap /mnt/hgfs/shared || echo ""

