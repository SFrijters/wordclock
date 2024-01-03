SHELL=/usr/bin/env bash
# This may need to be changed, based on the output of `arduino-cli board list`.
# Need permission to access this device, you may need to `sudo usermod -a -G dialout <username>`.
SERIAL=/dev/ttyUSB0
# This should be in sync with whatever you set in the sketch via `Serial.begin(xxx);` (if you set it at all).
BAUDRATE=115200
# This references the profile name in sketch.yaml .
PROFILE=wordclock

.PHONY: all spiffs compile upload monitor

all: spiffs compile upload

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
SPIFFS_IMG=/tmp/$(PROFILE).spiffs

spiffs:
	@echo Building SPIFFS image
	mkspiffs -c $(DATADIR) --page $(SPIFFS_PAGESIZE) --block $(SPIFFS_BLOCKSIZE) -s $(SPIFFS_SIZE) $(SPIFFS_IMG)
	mkspiffs -l            --page $(SPIFFS_PAGESIZE) --block $(SPIFFS_BLOCKSIZE) -s $(SPIFFS_SIZE) $(SPIFFS_IMG)
	esptool.py --chip esp8266 --baud 921600 --port $(SERIAL) erase_flash
	esptool.py --chip esp8266 --baud 921600 --port $(SERIAL) --before default_reset --after hard_reset write_flash --flash_size detect --flash_mode dio $(SPIFFS_START) $(SPIFFS_IMG)
