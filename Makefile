BIT = 32
ifeq ($(BIT),32)
	GCC_FLAG = -m32 -Os -ffunction-sections -fdata-sections -s -w -std=c11 -static
	ICO_FLAG = -F pe-i386
else
	GCC_FLAG = -Os -ffunction-sections -fdata-sections -s -w -std=c11 -static
	ICO_FLAG = 
endif

cabbage: main.c lib/mpc.c cabbage.ico.o cabbagew
	gcc main.c lib/mpc.c cabbage.ico.o -o cabbage $(GCC_FLAG)
cabbagew: main.c lib/mpc.c cabbage.ico.o
	gcc main.c lib/mpc.c cabbage.ico.o -o cabbagew $(GCC_FLAG) -mwindows
cabbage.ico.o: cabbage.rc
	windres cabbage.rc -o cabbage.ico.o $(ICO_FLAG)
clean:
	rm -f *.o