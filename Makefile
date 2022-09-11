GCC_PATH=/mnt/sda1/pvv/gcc/gcc-arm-11.2-2022.02-x86_64-arm-none-eabi/bin/

CPP_FLAGS = -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -fstack-usage -fcallgraph-info -std=c++17

all:
	$(GCC_PATH)arm-none-eabi-g++ -c funcs1.cpp funcs2.cpp $(CPP_FLAGS)
	g++ -std=c++17 main.cpp -o meas.elf
	./meas.elf funcs1 funcs2


.PHONY: clean
clean:
	-rm *.su *.ci *.o meas.elf
