CODAL_DIR = ../lib/codal
RM = /bin/rm
PYTHON3 ?= python3

SRC_HEX = $(CODAL_DIR)/MICROBIT.hex
SRC_MAP = $(CODAL_DIR)/build/MICROBIT.map
DEST_HEX = ./MICROBIT.hex

.PHONY: all clean

all:
	$(MAKE) -C codal_port
	git -C $(CODAL_DIR) checkout CMakeLists.txt build.py
	cat codal.patch | git -C $(CODAL_DIR) apply -
	(cd $(CODAL_DIR) && ./build.py)
	arm-none-eabi-size $(CODAL_DIR)/build/MICROBIT
	$(PYTHON3) addlayouttable.py $(SRC_HEX) $(SRC_MAP) -o $(DEST_HEX)

clean:
	$(MAKE) -C codal_port clean
	$(RM) -r $(CODAL_DIR)/build/
	$(RM) MICROBIT.hex
