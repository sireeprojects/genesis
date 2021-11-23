DBG ?= "off"
ifeq "$(DBG)" "on"
LIBPARAMS = -DCEA_DEBUG
else
LIBPARAMS =
endif

default:sim

sim:makelib
	@g++ test.cpp -o sim.x -lcdn_eth_proxy -L${PWD}
	@./sim.x

makelib:clean
	@g++ cdn_eth_proxy.cpp -s -fPIC -shared -o libcdn_eth_proxy.so -Wall -Wno-unused -lpthread

clean:
	@rm -rf *.x *.log *.so *.o
