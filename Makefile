DBG ?= "off"
ifeq "$(DBG)" "on"
LIBPARAMS = -DCEA_DEBUG
else
LIBPARAMS =
endif

default:sim

sim:makelib
	@clear; g++ test.cpp -o sim.x -lcea -L${PWD} ${LIBPARAMS}
	@./sim.x;
	@[ -f "run.pcap" ] && cp run.pcap /mnt/hgfs/shared || echo ""

makelib:clean
	@g++ cea.cpp -s -fPIC -shared -o libcea.so -Wall -Wno-unused -lpthread ${LIBPARAMS}

clean:
	@rm -rf *.x *.log *.so *.o run.pcap
