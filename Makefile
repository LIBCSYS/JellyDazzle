# dazzle64 — make        : build (regenerating tables if missing)
#            make gen    : force-regenerate palette.bin + sintab.bin
#            make run    : build and run
#            make clean  : remove binary and tables

CC      = clang
VERSION = $(shell cat VERSION)
VERFLAG = -DJD_VERSION='"$(VERSION)"'
# scheme count comes from palette.bin itself (131072 B per scheme), so
# doubling the palettes needs no edit in bridge.c
NSCHEMES = $(shell test -f palette.bin && expr $$(stat -f%z palette.bin) / 131072 || echo 30)
NSFLAG   = -DJD_NS=$(NSCHEMES)
# real SDL2 (NOT sdl2-compat, which dlopens SDL3 by name and breaks
# Finder launches with 'Failed loading SDL3 library')
SDL2DIR  = /opt/homebrew/opt/sdl2
SDLFLAGS = -I$(SDL2DIR)/include/SDL2 -D_THREAD_SAFE -L$(SDL2DIR)/lib -lSDL2 -Wl,-framework,Cocoa

# every .c under patterns_c except the standalone test harness — this is
# both the link list AND the prerequisite list, so regenerating registry.c
# or touching any pattern actually forces a relink (it used to not).
PATSRC = $(filter-out patterns_c/harness.c,$(wildcard patterns_c/*.c))

dazzle64: main.c bridge.c jellydazzle.h VERSION $(PATSRC) draw.s palette.bin sintab.bin shapes.bin
	$(CC) -O2 $(VERFLAG) $(NSFLAG) main.c bridge.c $(PATSRC) draw.s -o $@ $(SDLFLAGS)

palette.bin sintab.bin shapes.bin: gen_tables.py
	python3 gen_tables.py

.PHONY: gen run clean
gen:
	python3 gen_tables.py

run: dazzle64
	./dazzle64

clean:
	rm -f dazzle64 palette.bin sintab.bin shapes.bin
