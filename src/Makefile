CODAL_DIR = ../lib/codal
RM = /bin/rm

.PHONY: all clean

all:
	$(MAKE) -C codal_port
	git -C $(CODAL_DIR) checkout CMakeLists.txt
	cat codal.patch | git -C $(CODAL_DIR) apply -
	(cd $(CODAL_DIR) && ./build.py)
	cp $(CODAL_DIR)/MICROBIT.hex .

clean:
	$(MAKE) -C codal_port clean
	$(RM) -r $(CODAL_DIR)/build/
	$(RM) MICROBIT.hex
