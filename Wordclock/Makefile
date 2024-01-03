SHELL=/usr/bin/env bash
# This may need to be changed, based on the output of `arduino-cli board list`.
SERIAL=/dev/ttyUSB0
# This should be in sync with whatever you set in the sketch via `Serial.begin(xxx);` (if you set it at all).
BAUDRATE=115200
# This references the profile name in sketch.yaml .
PROFILE=wordclock

.PHONY: all compile upload monitor

all: compile upload

# Optional: provide a git hash so the running program can report what version it was built from.
#
# The arduino-cli executable runs in an FHS environment, but it downloads its own Python that does not work inside the `nix develop` shell.
# So we override it with a working Python from our nix development shell.
compile:
	arduino-cli compile -v \
	--profile $(PROFILE) \
	--build-property "compiler.cpp.extra_flags=\"-DGIT_VERSION=\"$(shell git rev-parse HEAD)\"\"" \
	--build-property "runtime.tools.python3.path=$(_ARDUINO_PYTHON3)/bin"

upload:
	[ -e $(SERIAL) ] && \
	arduino-cli upload -v -p $(SERIAL) --profile $(PROFILE) || \
	{ echo "==> $(SERIAL) is not available"; exit 1; }

# Monitor the serial output.
# The --imap option maps '\n' to '\r\n' so newlines are newlines.
monitor:
	picocom -b $(BAUDRATE) --imap lfcrlf $(SERIAL)

DATADIR     = $(CURDIR)/data/
SPIFFS_IMG  = /tmp/$(PROFILE).spiffs

# From arduino-cli compile --profile $(PROFILE) --show-properties

# arduino-cli compile -v --profile wordclock --show-properties | grep "^build.spiffs_blocksize"|cut -d= -f2
SPIFFS_BLOCKSIZE=8192
# arduino-cli compile -v --profile wordclock --show-properties | grep "^build.spiffs_pagesize"|cut -d= -f2
SPIFFS_PAGESIZE=256
# arduino-cli compile -v --profile wordclock --show-properties | grep "^build.spiffs_start"|cut -d= -f2
SPIFFS_START=0x200000
# arduino-cli compile -v --profile wordclock --show-properties | grep "^build.spiffs_end"|cut -d= -f2
SPIFFS_END=0x3FA000
# echo "ibase=16;${SPIFFS_END}-${SPIFFS_START}" | bc -q
SPIFFS_SIZE=2072576

# # Should be 1028096,  2076672, or  3125248 (1MB, 2MB, or 3MB)
# SPIFFS_SIZE = 1028096
# # Should be 0x300000 for 1MB, 0x200000 for 2MB, or 0x100000 for 3MB
# SPIFFS_ADDR = 0x200000

spiffs:
	@echo Building SPIFFS image
	PROPS=$$()
	BUILD_SPIFFS_BLOCKSIZE=$$(echo "$$PROPS"|grep "^build.spiffs_blocksize"|cut -d= -f2)
	echo BUILD_SPIFFS_SIZE $$BUILD_SPIFFS_SIZE
	mkspiffs -c $(DATADIR) --page $(SPIFFS_PAGESIZE) --block $(SPIFFS_BLOCKSIZE) -s $(SPIFFS_SIZE) $(SPIFFS_IMG)
	mkspiffs -l            --page $(SPIFFS_PAGESIZE) --block $(SPIFFS_BLOCKSIZE) -s $(SPIFFS_SIZE) $(SPIFFS_IMG)
	esptool.py --chip esp8266 --baud 921600 --port $(SERIAL) erase_flash
	esptool.py --chip esp8266 --before default_reset --after hard_reset --baud 921600 --port $(SERIAL) write_flash --flash_size detect --flash_mode dio $(SPIFFS_START) $(SPIFFS_IMG)
