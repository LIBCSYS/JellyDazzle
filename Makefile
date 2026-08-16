# JellyDazzle — build
#   make            build ./jellydazzle
#   make run        build + launch
#   make assets     regenerate palette tables from assets/palettes/*
#   make registry   regenerate the pattern registry
#   make app        signed self-contained JellyDazzle.app (tools/build_app.sh)
#   make clean

CC       = clang
VERSION  = $(shell cat VERSION)
# scheme count comes from the generated palette_count.h (JD_SCHEMES) so the
# compositor's palette bag covers EVERY scheme in palette.bin (2.3 passed
# -DJD_NS from the .bin size; the port dropped it and fell back to 30/180)
NSCHEMES = $(shell awk '/define JD_SCHEMES/{print $$3}' src/engine/palette_count.h)
CFLAGS   = -O2 -Isrc/engine -DJD_VERSION='"$(VERSION)"' -DJD_NS=$(NSCHEMES)
SDLFLAGS = $(shell sdl2-config --cflags --libs)

ENGINE   = src/engine/compositor.c src/engine/routines_asm.s
AUDIO    = src/audio/listen.c src/audio/systap.m
# systap.m: Core Audio process tap (system-output capture) — needs these frameworks
AUDIOLIB = -framework CoreAudio -framework Foundation -lobjc
APP      = src/app/main.c
PATTERNS = $(filter-out src/patterns/_harness.c,$(wildcard src/patterns/[0-9]*.c)) src/patterns/_registry.c
ASSETS   = assets/palette.bin assets/sintab.bin src/engine/palette_count.h

jellydazzle: VERSION $(APP) $(AUDIO) $(ENGINE) $(PATTERNS) src/engine/jellydazzle.h $(ASSETS)
	$(CC) $(CFLAGS) $(APP) $(AUDIO) $(ENGINE) $(PATTERNS) -o $@ $(SDLFLAGS) $(AUDIOLIB)

$(ASSETS): tools/gen_palettes.py assets/palettes/lospec.json $(wildcard assets/palettes/designed/*.json)
	python3 tools/gen_palettes.py

src/patterns/_registry.c: $(wildcard src/patterns/[0-9]*.c)
	tools/gen_registry.sh

.PHONY: run assets registry app clean
run: jellydazzle
	./jellydazzle
assets:
	python3 tools/gen_palettes.py
registry:
	tools/gen_registry.sh
app: jellydazzle
	tools/build_app.sh
clean:
	rm -f jellydazzle src/patterns/*.o
