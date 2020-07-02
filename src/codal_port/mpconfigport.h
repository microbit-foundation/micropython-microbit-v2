/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// Options to control how MicroPython is built.

#include <stdint.h>

#define MICROPY_LONGINT_IMPL                    (MICROPY_LONGINT_IMPL_MPZ)
#define MICROPY_FLOAT_IMPL                      (MICROPY_FLOAT_IMPL_FLOAT)
#define MICROPY_ALLOC_PATH_MAX                  (PATH_MAX)
#define MICROPY_ENABLE_GC                       (1)
#define MICROPY_KBD_EXCEPTION                   (1)
#define MICROPY_HELPER_REPL                     (1)
#define MICROPY_REPL_AUTO_INDENT                (1)
#define MICROPY_MODULE_BUILTIN_INIT             (1)
#define MICROPY_PY_BUILTINS_MEMORYVIEW          (1)
#define MICROPY_PY___FILE__                     (0)
#define MICROPY_PY_SYS_MAXSIZE                  (1)
#define MICROPY_PY_SYS_PLATFORM                 "microbit"

#define MICROPY_HW_ENABLE_RNG                   (1)
#define MICROPY_MBFS                            (1)

// extra built in names to add to the global namespace
#define MICROPY_PORT_BUILTINS \
    { MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&mp_builtin_open_obj) },

#define MICROPY_HW_BOARD_NAME "microbit"
#define MICROPY_HW_MCU_NAME "nRF52"

#define MP_STATE_PORT MP_STATE_VM

extern const struct _mp_obj_module_t microbit_module;
extern const struct _mp_obj_module_t music_module;
extern const struct _mp_obj_module_t os_module;

#define MICROPY_PORT_BUILTIN_MODULES \
    { MP_ROM_QSTR(MP_QSTR_microbit), MP_ROM_PTR(&microbit_module) }, \
    { MP_ROM_QSTR(MP_QSTR_music), MP_ROM_PTR(&music_module) }, \
    { MP_ROM_QSTR(MP_QSTR_os), MP_ROM_PTR(&os_module) }, \

#define MICROPY_PORT_ROOT_POINTERS \
    const char *readline_hist[8]; \
    void *async_data[2]; \
    struct _music_data_t *music_data; \

#define MP_SSIZE_MAX (0x7fffffff)

// Type definitions for the specific machine
typedef intptr_t mp_int_t; // must be pointer size
typedef uintptr_t mp_uint_t; // must be pointer size
typedef long mp_off_t;

// We need to provide a declaration/definition of alloca()
#include <alloca.h>
