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
basket: pkg/basket.c
	gcc pkg/basket.c -o basket $(GCC_FLAG) -lcurl
cabbage: main.c lib/raylib_cbg.c basket
	gcc main.c -o cabbage lib/raylib_cbg.c $(GCC_FLAG) -lm -ledit -lcurl -lraylib

cabbage.ico.o: cabbage.rc
	windres cabbage.rc -o cabbage.ico.o $(ICO_FLAG)
windows: cabbage.exe cabbagew.exe
	@echo $(BIT)-bit compiled successfully.
basket.exe: pkg/basket.c
	gcc pkg/basket.c -o basket $(GCC_FLAG) -lwininet
cabbage.exe: main.c lib/raylib_cbg.c cabbage.ico.o cabbagew.exe
	gcc main.c lib/raylib_cbg.c cabbage.ico.o -o cabbage $(GCC_FLAG) -lws2_32 -lraylib -lwinmm -lwininet -lgdi32 -lopengl32 -static
cabbagew.exe: main.c lib/raylib_cbg.c cabbage.ico.o basket.exe
	gcc main.c lib/raylib_cbg.c cabbage.ico.o -o cabbagew $(GCC_FLAG) -mwindows  -lws2_32 -lraylib -lwinmm -lwininet -lgdi32 -lopengl32 -static

clean:
	rm -f cabbage.exe cabbagew.exe basket.exe cabbage basket cabbage.ico.o
