ifeq ($(OS),Windows_NT)
	WMIC_OS_BIT := $(strip $(shell wmic os get osarchitecture))
	ifeq ($(WMIC_OS_BIT),OSArchitecture 64-bit)
		BIT := 64
	else
		BIT := 32
	endif
else
    BIT := $(shell getconf LONG_BIT)
endif

ifeq ($(BIT),32)
	GCC_FLAG = -m32 -Os -ffunction-sections -fdata-sections -s -w -std=c11
	ICO_FLAG = -F pe-i386
else
	GCC_FLAG = -Os -ffunction-sections -fdata-sections -s -w -std=c11
	ICO_FLAG = 
endif

unix: cabbage
	@echo "$(BIT)-bit compiled successfully."
cabbage: main.c
	gcc main.c -o cabbage $(GCC_FLAG) -lm -lreadline

cabbage.ico.o: cabbage.rc
	windres cabbage.rc -o cabbage.ico.o $(ICO_FLAG)
windows: cabbage.exe cabbagew.exe
	@echo $(BIT)-bit compiled successfully.
cabbage.exe: main.c cabbage.ico.o cabbagew.exe
	gcc main.c cabbage.ico.o -o cabbage $(GCC_FLAG) -lws2_32 -lwinmm -lwininet -static
cabbagew.exe: main.c cabbage.ico.o
	gcc main.c cabbage.ico.o -o cabbagew $(GCC_FLAG) -mwindows  -lws2_32 -lwinmm -lwininet -static

clean:
	rm -f cabbage.exe cabbagew.exe cabbage cabbage.ico.o
