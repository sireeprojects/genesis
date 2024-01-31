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

sim:makelib
	@g++ -o test.exe test.cpp libcea.so
	@test.exe

makelib:clean
	@g++ -c cea.cpp -o cea.o
	@g++ -shared -o libcea.so cea.o

clean:
	@rm -rf *.x *.log *.so *.o *.pcap *.exe
