CC ?= cc
CFLAGS ?= -O2 -fPIC -Wall -Wextra -std=c99 -fno-strict-aliasing
LV2_CFLAGS ?= $(shell pkg-config --cflags lv2 2>/dev/null)

UNAME_S := $(shell uname -s 2>/dev/null)
ifeq ($(OS),Windows_NT)
  PLUGIN_EXT := dll
  SHARED_FLAGS := -shared
else ifneq (,$(findstring MINGW,$(UNAME_S)))
  PLUGIN_EXT := dll
  SHARED_FLAGS := -shared
else ifeq ($(UNAME_S),Darwin)
  PLUGIN_EXT := dylib
  SHARED_FLAGS := -bundle -undefined dynamic_lookup
else
  PLUGIN_EXT := so
  SHARED_FLAGS := -shared
endif

MACOS_ARCHS ?=
PLUGIN := talkingbass.$(PLUGIN_EXT)
BUNDLE_DIR := build/talkingbass.lv2

.PHONY: all bundle clean

all: $(PLUGIN)

$(PLUGIN): src/talkingbass.c
	$(CC) $(CFLAGS) $(LV2_CFLAGS) $(MACOS_ARCHS) $(SHARED_FLAGS) -o $@ $< -lm

bundle: $(PLUGIN)
	mkdir -p "$(BUNDLE_DIR)"
	cp "$(PLUGIN)" "$(BUNDLE_DIR)/"
	cp talkingbass.ttl "$(BUNDLE_DIR)/"
	sed 's/talkingbass\.so/talkingbass.$(PLUGIN_EXT)/g' manifest.ttl > "$(BUNDLE_DIR)/manifest.ttl"

clean:
	rm -f talkingbass.so talkingbass.dylib talkingbass.dll
