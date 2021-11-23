PROJECT_ROOT = /root/genesis
DBG ?= "off"
ifeq "$(DBG)" "on"
LIBPARAMS = -DCEA_DEBUG
else
LIBPARAMS =
endif

default:sim

sim:makelib
	@g++ -O3 \
		${PROJECT_ROOT}/test/test.cpp \
		-I${PROJECT_ROOT}/src \
		-o sim.x \
		-lcea \
		-L${PWD}
	@./sim.x

makelib:clean
	@g++ -O3 ${PROJECT_ROOT}/src/cea.cpp -I${PROJECT_ROOT}/src \
		-s -fPIC -shared -o libcea.so -Wall -Wno-unused -lpthread

clean:
	@rm -rf *.x *.log *.so *.o
