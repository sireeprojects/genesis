DBG ?= on

ifeq "$(DBG)" "on"
LIBPARAMS += -DCEA_DEBUG
else
LIBPARAMS +=
endif

sim:makelib
	@g++ test.cpp -o sim.x -lcea -L${PWD} ${LIBPARAMS}
	@./sim.x;

makelib:clean
	@clear; g++ cea.cpp -s -fPIC -shared -o libcea.so -Wall -Wno-unused -lpthread ${LIBPARAMS}

clean:
	@rm -rf *.x *.log *.so *.o run.pcap

copy:
	@[ -f "run.pcap" ] && cp run.pcap /mnt/hgfs/shared || echo ""
