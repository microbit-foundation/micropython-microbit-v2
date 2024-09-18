# Makefile to build libmicropython.a

MICROPY_ROM_TEXT_COMPRESSION ?= 1
FROZEN_MANIFEST ?= manifest.py

CROSS_COMPILE = arm-none-eabi-
CFLAGS_EXTRA = -mthumb -mtune=cortex-m4 -mcpu=cortex-m4

include ../../lib/micropython/py/mkenv.mk
-include mpconfigport.mk

# QSTR definitions (must come before including py.mk).
QSTR_DEFS = qstrdefsport.h

# Include py core make definitions.
include $(TOP)/py/py.mk
include $(TOP)/extmod/extmod.mk

# The micropython-lib submodule is not needed by this project, but the MicroPython
# build system requires it if FROZEN_MANIFEST is set (which it is below).  To avoid
# needing to check out the micropython-lib submodule, point MPY_LIB_DIR to a dummy
# location that has a README.md file.
MPY_LIB_DIR = $(TOP)

MP_VER_FILE = $(HEADER_BUILD)/mpversion.h
MBIT_VER_FILE = $(HEADER_BUILD)/microbitversion.h

LOCAL_LIB_DIR = ../../lib
CMSIS_DIR = $(LOCAL_LIB_DIR)/codal/libraries/codal-nrf52/inc/cmsis
NRFX_DIR = $(LOCAL_LIB_DIR)/codal/libraries/codal-nrf52/nrfx

INC += -I.
INC += -I../codal_app
INC += -I$(LOCAL_LIB_DIR)
INC += -I$(CMSIS_DIR)
INC += -I$(NRFX_DIR)
INC += -I$(NRFX_DIR)/drivers/include
INC += -I$(NRFX_DIR)/hal
INC += -I$(NRFX_DIR)/mdk
INC += -I$(TOP)
INC += -I$(TOP)/ports/nrf
INC += -I$(BUILD)

# Compiler settings.
CWARN = -Wall -Werror
CWARN += -Wpointer-arith -Wuninitialized
CFLAGS_ARCH += -DNRF52833_XXAA
CFLAGS += $(INC) $(CWARN) -std=c99 $(CFLAGS_MOD) $(CFLAGS_ARCH) $(COPT) $(CFLAGS_EXTRA)
CXXFLAGS += $(filter-out -std=c99,$(CFLAGS))

# Debugging/Optimization
ifdef DEBUG
COPT = -O0
else
COPT = -Os -DNDEBUG
endif

CFLAGS += -g
LDFLAGS_ARCH = -Wl,-map,$@.map
LDFLAGS += $(LDFLAGS_MOD) $(LDFLAGS_ARCH) -lm $(LDFLAGS_EXTRA)

SRC_C += \
	drv_display.c \
	drv_image.c \
	drv_radio.c \
	drv_softtimer.c \
	drv_system.c \
	help.c \
	iters.c \
	main.c \
	microbit_accelerometer.c \
	microbit_audiorecording.c \
	microbit_audiotrack.c \
	microbit_button.c \
	microbit_compass.c \
	microbit_display.c \
	microbit_i2c.c \
	microbit_image.c \
	microbit_constimage.c \
	microbit_microphone.c \
	microbit_pin.c \
	microbit_pinaudio.c \
	microbit_pinmode.c \
	microbit_sound.c \
	microbit_soundeffect.c \
	microbit_soundevent.c \
	microbit_speaker.c \
	microbit_spi.c \
	microbit_uart.c \
	microbitfs.c \
	modantigravity.c \
	modaudio.c \
	modlog.c \
	modlove.c \
	modmachine.c \
	modmicrobit.c \
	modmusic.c \
	modmusictunes.c \
	modos.c \
	modpower.c \
	modradio.c \
	modspeech.c \
	modthis.c \
	mphalport.c \
	utils.c \

SRC_C += \
	shared/readline/readline.c \
	shared/runtime/gchelper_native.c \
	shared/runtime/pyexec.c \
	shared/runtime/stdout_helpers.c \
	$(abspath $(NRFX_DIR)/drivers/src/nrfx_nvmc.c) \
	$(abspath $(LOCAL_LIB_DIR)/sam/main.c) \
	$(abspath $(LOCAL_LIB_DIR)/sam/reciter.c) \
	$(abspath $(LOCAL_LIB_DIR)/sam/render.c) \
	$(abspath $(LOCAL_LIB_DIR)/sam/sam.c) \
	$(abspath $(LOCAL_LIB_DIR)/sam/debug.c) \

SRC_O += \
	shared/runtime/gchelper_thumb2.o \

OBJ = $(PY_O)
OBJ += $(addprefix $(BUILD)/, $(SRC_O))
OBJ += $(addprefix $(BUILD)/, $(SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(LIB_SRC_C:.c=.o))

# List of sources for qstr extraction.
SRC_QSTR += $(SRC_C) $(LIB_SRC_C)
# Append any auto-generated sources that are needed by sources listed in.
# SRC_QSTR
SRC_QSTR_AUTO_DEPS +=
QSTR_GLOBAL_REQUIREMENTS += $(MBIT_VER_FILE)

# Top-level rule.
all: lib $(MBIT_VER_FILE)

# Rule to build header with micro:bit specific version information.
# Also rebuild MicroPython version header in correct directory to pick up git hash.
$(MBIT_VER_FILE): FORCE
	(cd $(TOP) && $(PYTHON) py/makeversionhdr.py $(abspath $(MP_VER_FILE)))
	$(PYTHON) make_microbit_version_hdr.py $(MBIT_VER_FILE)

# Suppress warnings from SAM library.
$(BUILD)/$(abspath $(LOCAL_LIB_DIR))/sam/sam.o: CWARN += -Wno-array-bounds

include $(TOP)/py/mkrules.mk
