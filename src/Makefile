CODAL_DIR = ../lib/codal
RM = /bin/rm
MKDIR = /bin/mkdir
PYTHON ?= python3

BUILD = ./build
CODAL_BUILD = $(CODAL_DIR)/build
CODAL_LIBRARIES = $(CODAL_DIR)/libraries
SRC_HEX = $(BUILD)/MICROBIT.hex
SRC_MAP = $(CODAL_BUILD)/MICROBIT.map
DEST_HEX = ./MICROBIT.hex

define CODAL_CLEAN
	git -C $(CODAL_DIR) checkout CMakeLists.txt
endef

define CODAL_PATCH
	$(call CODAL_CLEAN)
	cat codal.patch | git -C $(CODAL_DIR) apply -
endef

define CODAL_LIBRARIES_CODAL_NRF52_CLEAN
	git -C $(CODAL_DIR)/libraries/codal-nrf52 checkout inc
endef

define CODAL_LIBRARIES_CODAL_NRF52_PATCH
	$(call CODAL_LIBRARIES_CODAL_NRF52_CLEAN)
	cat neopixel_ws2812b_timing.patch | git -C $(CODAL_DIR)/libraries/codal-nrf52 apply -
endef

.PHONY: all codal_cmake codal_build libmicropython clean

all: codal_build

# Create output directory, run cmake, and make sure codal libraries exist (via cmake)
codal_cmake:
	$(MKDIR) -p $(BUILD)
	$(call CODAL_PATCH)
	(cd $(BUILD) && cmake ../$(CODAL_DIR) -DCMAKE_BUILD_TYPE=MinSizeRel)
	$(call CODAL_CLEAN)

# Build the codal app and make the final HEX file
codal_build: libmicropython
	$(call CODAL_PATCH)
	$(call CODAL_LIBRARIES_CODAL_NRF52_PATCH)
	make -C $(BUILD)
	$(call CODAL_LIBRARIES_CODAL_NRF52_CLEAN)
	$(call CODAL_CLEAN)
	arm-none-eabi-size $(CODAL_BUILD)/MICROBIT
	$(PYTHON) addlayouttable.py $(SRC_HEX) $(SRC_MAP) -o $(DEST_HEX)

# Build the MicroPython component
libmicropython: $(CODAL_LIBRARIES) $(BUILD)
	$(call CODAL_CLEAN)
	$(MAKE) -C codal_port

$(CODAL_LIBRARIES) $(BUILD): codal_cmake

clean:
	$(MAKE) -C codal_port clean
	$(RM) -rf $(BUILD)
	$(RM) -rf $(CODAL_BUILD)
	$(RM) -rf $(CODAL_LIBRARIES)
	$(RM) MICROBIT.hex
