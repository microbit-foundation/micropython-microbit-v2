# Makefile to build libmicropython.a

CROSS_COMPILE = arm-none-eabi-
CFLAGS_EXTRA = -mthumb -mtune=cortex-m4 -mcpu=cortex-m4

include ../../lib/micropython/py/mkenv.mk
-include mpconfigport.mk

all: lib

# QSTR definitions (must come before including py.mk).
QSTR_DEFS = qstrdefsport.h

# Include py core make definitions.
include $(TOP)/py/py.mk

INC += -I.
INC += -I../codal_app
INC += -I$(TOP)
INC += -I$(BUILD)

# Compiler settings.
CWARN = -Wall -Werror
CWARN += -Wpointer-arith -Wuninitialized
CFLAGS = $(INC) $(CWARN) -std=c99 $(CFLAGS_MOD) $(COPT) $(CFLAGS_EXTRA)

# Debugging/Optimization
ifdef DEBUG
COPT = -O0
else
COPT = -Os -DNDEBUG
endif

CFLAGS += -g
LDFLAGS_ARCH = -Wl,-map,$@.map
LDFLAGS = $(LDFLAGS_MOD) $(LDFLAGS_ARCH) -lm $(LDFLAGS_EXTRA)

SRC_C += \
	drv_display.c \
	drv_image.c \
	drv_system.c \
	iters.c \
	main.c \
	microbit_accelerometer.c \
	microbit_button.c \
	microbit_display.c \
	microbit_image.c \
	microbit_constimage.c \
	microbit_pin.c \
	microbit_pinmode.c \
	modmicrobit.c \
	modmusic.c \
	modmusictunes.c \
	mphalport.c \

SRC_C += \
	lib/mp-readline/readline.c \
	lib/utils/gchelper_native.c \
	lib/utils/pyexec.c \
	lib/utils/stdout_helpers.c \

SRC_O += \
	lib/utils/gchelper_m3.o \

OBJ = $(PY_O)
OBJ += $(addprefix $(BUILD)/, $(SRC_O))
OBJ += $(addprefix $(BUILD)/, $(SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(LIB_SRC_C:.c=.o))

# List of sources for qstr extraction.
SRC_QSTR += $(SRC_C) $(LIB_SRC_C)
# Append any auto-generated sources that are needed by sources listed in.
# SRC_QSTR
SRC_QSTR_AUTO_DEPS +=

include $(TOP)/py/mkrules.mk

CLEAN_EXTRA += $(LIBMICROPYTHON)
