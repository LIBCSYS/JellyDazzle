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
# Deployment target. Without this the binary inherits whatever macOS it was
# built on — a build made on macOS 26 refuses to launch on anything older, so
# the public download only worked for people already on the newest OS. The
# Core Audio process tap is guarded by a runtime @available(macOS 14.2) check
# and falls back to the microphone, so nothing here needs a modern OS.
MACMIN   = 11.0
CFLAGS   = -O2 -mmacosx-version-min=$(MACMIN) -Isrc/engine -DJD_VERSION='"$(VERSION)"' -DJD_NS=$(NSCHEMES)
# Prefer a portable SDL2 built against MACMIN. Homebrew's SDL is compiled for
# whatever macOS the machine runs, so bundling it pins the app to that OS no
# matter what we target — the app said macOS 11 while the dylib beside it said
# 26. tools/build_sdl.sh produces this; falls back to Homebrew if absent.
SDL2_PORTABLE = vendor/sdl2
SDLFLAGS = $(if $(wildcard $(SDL2_PORTABLE)/lib/libSDL2-2.0.0.dylib),\
             -I$(SDL2_PORTABLE)/include/SDL2 -D_THREAD_SAFE -L$(SDL2_PORTABLE)/lib -lSDL2 -Wl$(comma)-framework$(comma)Cocoa,\
             $(shell sdl2-config --cflags --libs))
comma := ,
LDFLAGS += -mmacosx-version-min=$(MACMIN)

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
