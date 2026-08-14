# dazzle64 — make        : build (regenerating tables if missing)
#            make gen    : force-regenerate palette.bin + sintab.bin
#            make run    : build and run
#            make clean  : remove binary and tables

CC      = clang
SDLFLAGS = $(shell sdl2-config --cflags --libs)

dazzle64: main.c draw.s palette.bin sintab.bin shapes.bin
	$(CC) main.c draw.s -o $@ $(SDLFLAGS)

palette.bin sintab.bin shapes.bin: gen_tables.py
	python3 gen_tables.py

.PHONY: gen run clean
gen:
	python3 gen_tables.py

run: dazzle64
	./dazzle64

clean:
	rm -f dazzle64 palette.bin sintab.bin
