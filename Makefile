BIT = 64
GCC_FLAG = -Os -ffunction-sections -fdata-sections -s -w -std=c11
ICO_FLAG = 

windows: cabbage.exe cabbagew.exe
	@echo Done.
cabbage.exe: main.c lib/mpc.c cabbage.ico.o cabbagew.exe
	gcc main.c lib/mpc.c cabbage.ico.o -o cabbage $(GCC_FLAG) -lws2_32 -static
cabbagew.exe: main.c lib/mpc.c cabbage.ico.o
	gcc main.c lib/mpc.c cabbage.ico.o -o cabbagew $(GCC_FLAG) -mwindows  -lws2_32 -static

unix: cabbage
	@echo "Done."
cabbage: main.c lib/mpc.c
	gcc main.c lib/mpc.c -o cabbage $(GCC_FLAG) -lm -lreadline
cabbage.ico.o: cabbage.rc
	windres cabbage.rc -o cabbage.ico.o $(ICO_FLAG)
