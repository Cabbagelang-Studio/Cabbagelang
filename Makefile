BIT = 32
ifeq ($(BIT),32)
	GCC_FLAG = -m32 -Os -ffunction-sections -fdata-sections -s -w -std=c11 -static
	ICO_FLAG = -F pe-i386
else
	GCC_FLAG = -Os -ffunction-sections -fdata-sections -s -w -std=c11 -static
	ICO_FLAG = 
endif

windows: cabbage.exe cabbagew.exe
	@echo Done.
cabbage.exe: main.c lib/mpc.c cabbage.ico.o cabbagew.exe
	gcc main.c lib/mpc.c cabbage.ico.o -o cabbage $(GCC_FLAG) -lws2_32
cabbagew.exe: main.c lib/mpc.c cabbage.ico.o
	gcc main.c lib/mpc.c cabbage.ico.o -o cabbagew $(GCC_FLAG) -mwindows  -lws2_32

unix: cabbage
	@echo "Done."
cabbage: main.c lib/mpc.c cabbage.ico.o
	gcc main.c lib/mpc.c cabbage.ico.o -o cabbage $(GCC_FLAG) -lm -lreadline
cabbage.ico.o: cabbage.rc
	windres cabbage.rc -o cabbage.ico.o $(ICO_FLAG)