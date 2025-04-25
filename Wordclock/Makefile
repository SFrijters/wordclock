SHELL=/usr/bin/env bash
# This may need to be changed, based on the output of `arduino-cli board list`.
# Need permission to access this device, you may need to `sudo usermod -a -G dialout <username>`.
SERIAL=/dev/ttyUSB0
# This should be in sync with whatever you set in the sketch via `Serial.begin(xxx);` (if you set it at all).
BAUDRATE=115200
FQBN=esp8266:esp8266:d1_mini

VERBOSE=

ROOT_DIR:=$(shell pwd)
SKETCH_NAME:=$(shell basename $(ROOT_DIR))

COMPILER_CPP_EXTRA_FLAGS=

GIT_VERSION:=$(shell git rev-parse HEAD)
GIT_VERSION_SHORT:=$(shell git rev-parse --short HEAD)
GIT_DIRTY:=$(shell git diff --no-ext-diff --quiet || echo "*")
GIT_ADDED:=$(shell git diff --no-ext-diff --cached --quiet || echo "+")

_ARDUINO_ROOT_DIR=/tmp/arduino/
_ARDUINO_PROJECT_DIR=$(_ARDUINO_ROOT_DIR)/$(SKETCH_NAME)
_ARDUINO_BUILD_DIR=$(_ARDUINO_PROJECT_DIR)/build/$(SKETCH_NAME)
_ARDUINO_OUTPUT_DIR=$(_ARDUINO_PROJECT_DIR)/bin/$(SKETCH_NAME)

.PHONY: all spiffs compile upload monitor clean

all: spiffs compile upload

# commonhfile.fqfn needs to be set because it needs to be in a writable directory
compile:
	arduino-cli compile "$(VERBOSE)" \
	--fqbn "$(FQBN)" \
	--build-property "compiler.cpp.extra_flags=$(COMPILER_CPP_EXTRA_FLAGS) -DSKETCH_NAME=\"$(SKETCH_NAME)\" -DGIT_VERSION=\"$(GIT_VERSION)$(GIT_DIRTY)$(GIT_ADDED)\" -DGIT_VERSION_SHORT=\"$(GIT_VERSION_SHORT)$(GIT_DIRTY)$(GIT_ADDED)\" $(EXTRA_FLAGS)" \
	--build-property "commonhfile.fqfn=$(_ARDUINO_BUILD_DIR)/CommonHFile.h" \
	--build-path "$(_ARDUINO_BUILD_DIR)" \
	--output-dir "$(_ARDUINO_OUTPUT_DIR)"

upload:
	[ -e $(SERIAL) ] && \
	arduino-cli upload \
	--fqbn "$(FQBN)" \
	--input-dir "$(_ARDUINO_OUTPUT_DIR)" \
        -p "$(SERIAL)" \
        -v || \
	{ echo "==> $(SERIAL) is not available"; exit 1; }

# Monitor the serial output.
# The --imap option maps '\n' to '\r\n' so newlines are newlines.
monitor:
	picocom -b $(BAUDRATE) --imap lfcrlf $(SERIAL)

# From arduino-cli compile --profile $(PROFILE) --show-properties
# arduino-cli compile -v --profile wordclock --show-properties | grep "^build.spiffs_blocksize" | cut -d= -f2
SPIFFS_BLOCKSIZE=8192
# arduino-cli compile -v --profile wordclock --show-properties | grep "^build.spiffs_pagesize" | cut -d= -f2
SPIFFS_PAGESIZE=256
# arduino-cli compile -v --profile wordclock --show-properties | grep "^build.spiffs_start" | cut -d= -f2
SPIFFS_START=0x200000
# arduino-cli compile -v --profile wordclock --show-properties | grep "^build.spiffs_end" | cut -d= -f2
SPIFFS_END=0x3FA000
# echo "ibase=16;${SPIFFS_END//0x/}-${SPIFFS_START//0x/}" | bc -q
SPIFFS_SIZE=2072576
DATADIR=$(CURDIR)/data/
SPIFFS_IMG=/tmp/wordclock.spiffs

spiffs:
	@echo Building SPIFFS image
	mkspiffs -c $(DATADIR) --page $(SPIFFS_PAGESIZE) --block $(SPIFFS_BLOCKSIZE) -s $(SPIFFS_SIZE) $(SPIFFS_IMG)
	mkspiffs -l            --page $(SPIFFS_PAGESIZE) --block $(SPIFFS_BLOCKSIZE) -s $(SPIFFS_SIZE) $(SPIFFS_IMG)
	esptool.py --chip esp8266 --baud 921600 --port $(SERIAL) erase_flash
	esptool.py --chip esp8266 --baud 921600 --port $(SERIAL) --before default_reset --after hard_reset write_flash --flash_size detect --flash_mode dio $(SPIFFS_START) $(SPIFFS_IMG)
clean:
	rm -rf "$(_ARDUINO_OUTPUT_DIR)"
	rm -rf "$(_ARDUINO_BUILD_DIR)"
