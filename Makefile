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
# 11.0 for the direct download — widest reach on Apple Silicon.
# ⚠ The APP STORE build overrides this to 12.0: Apple rejects an arm64-only
# bundle (error 90869) unless the deployment target is 12.0 or higher. Intel
# Macs cannot run this anyway — the renderer is ARM64 assembly — so raising the
# floor costs nothing there, but it does drop macOS 11 users from the store
# build, which is why the download keeps 11.0.
MACMIN  ?= 11.0
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
# menu_mac.m: the native NSMenu menu bar - needs AppKit. SDLFLAGS only carries
# -framework Cocoa on the vendored-SDL branch; the sdl2-config fallback does not,
# so link it explicitly here rather than depending on which branch fired.
UI       = src/app/menu_mac.m
UILIB    = -framework Cocoa
APP      = src/app/main.c
PATTERNS = $(filter-out src/patterns/_harness.c,$(wildcard src/patterns/[0-9]*.c)) src/patterns/_registry.c
ASSETS   = assets/palette.bin assets/sintab.bin src/engine/palette_count.h

jellydazzle: VERSION .macmin $(APP) $(UI) $(AUDIO) $(ENGINE) $(PATTERNS) src/engine/jellydazzle.h $(ASSETS)
	$(CC) $(CFLAGS) $(APP) $(UI) $(AUDIO) $(ENGINE) $(PATTERNS) -o $@ $(SDLFLAGS) $(AUDIOLIB) $(UILIB)

$(ASSETS): tools/gen_palettes.py assets/palettes/lospec.json $(wildcard assets/palettes/designed/*.json)
	python3 tools/gen_palettes.py

src/patterns/_registry.c: $(wildcard src/patterns/[0-9]*.c)
	tools/gen_registry.sh

# MACMIN is a compiler flag, not a file, so make cannot see it change. Switching
# from the download build (11.0) to the App Store build (12.0) therefore left the
# PREVIOUS binary in place while build_app.sh wrote the new floor into Info.plist
# — measured: plist 12.0, binary still 11.0. That mismatch looks fine locally and
# is what Apple rejects on upload. This stamp turns the flag into a dependency.
# It rewrites only when the value actually changes, so ordinary builds are
# untouched and no-op rebuilds stay no-ops.
.macmin: FORCE
	@[ "$$(cat $@ 2>/dev/null)" = "$(MACMIN)" ] || echo "$(MACMIN)" > $@

.PHONY: run assets registry app clean FORCE
FORCE:
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
