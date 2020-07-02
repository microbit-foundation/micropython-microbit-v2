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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "py/gc.h"
#include "py/runtime.h"
#include "py/stackctrl.h"
#include "lib/utils/gchelper.h"
#include "lib/utils/pyexec.h"
#include "ports/nrf/modules/uos/microbitfs.h"
#include "drv_display.h"

// Use a fixed static buffer for the heap.
static char heap[64 * 1024];

void mp_main(void) {
    mp_stack_ctrl_init();
    mp_stack_set_limit(8 * 1024);

    for (;;) {
        microbit_display_init();
        microbit_filesystem_init();

        gc_init(heap, heap + sizeof(heap));
        mp_init();

        mp_obj_list_init(MP_OBJ_TO_PTR(mp_sys_path), 0);
        mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR_));
        mp_obj_list_init(MP_OBJ_TO_PTR(mp_sys_argv), 0);

        if (pyexec_mode_kind == PYEXEC_MODE_FRIENDLY_REPL) {
            // from microbit import *
            mp_import_all(mp_import_name(MP_QSTR_microbit, mp_const_empty_tuple, MP_OBJ_NEW_SMALL_INT(0)));
        }

        for (;;) {
            if (pyexec_mode_kind == PYEXEC_MODE_RAW_REPL) {
                if (pyexec_raw_repl() != 0) {
                    break;
                }
            } else {
                if (pyexec_friendly_repl() != 0) {
                    break;
                }
            }
        }

        mp_printf(MP_PYTHON_PRINTER, "MPY: soft reboot\n");
        gc_sweep_all();
        mp_deinit();
    }
}

#if MICROPY_MBFS
mp_lexer_t *mp_lexer_new_from_file(const char *filename) {
    return uos_mbfs_new_reader(filename);
}

mp_import_stat_t mp_import_stat(const char *path) {
    return uos_mbfs_import_stat(path);
}

STATIC mp_obj_t mp_builtin_open(size_t n_args, const mp_obj_t *args) {
    return uos_mbfs_open(n_args, args);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_builtin_open_obj, 1, 2, mp_builtin_open);
#endif

void gc_collect(void) {
    gc_collect_start();
    gc_helper_collect_regs_and_stack();
    gc_collect_end();
}

void nlr_jump_fail(void *val) {
    printf("FATAL: uncaught NLR %p\n", val);
    exit(1);
}

// For debugging
int m_printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = mp_vprintf(&mp_plat_print, fmt, ap);
    va_end(ap);
    return ret;
}
