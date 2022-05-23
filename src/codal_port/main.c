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

#include "py/compile.h"
#include "py/gc.h"
#include "py/mphal.h"
#include "py/runtime.h"
#include "py/stackctrl.h"
#include "shared/readline/readline.h"
#include "shared/runtime/gchelper.h"
#include "shared/runtime/pyexec.h"
#include "ports/nrf/modules/uos/microbitfs.h"
#include "drv_softtimer.h"
#include "drv_system.h"
#include "drv_display.h"
#include "modmicrobit.h"

// Use a fixed static buffer for the heap.
static char heap[64 * 1024];

// Set to true if a soft-timer callback can use mp_sched_exception to propagate out an exception.
bool microbit_outer_nlr_will_handle_soft_timer_exceptions;

void microbit_pyexec_file(const char *filename);

void mp_main(void) {
    mp_stack_ctrl_init();
    mp_stack_set_limit(8 * 1024 - 512); // include 512 byte buffer zone for overflow

    for (;;) {
        microbit_system_init();
        microbit_display_init();
        #if MICROPY_MBFS
        microbit_filesystem_init();
        #endif

        gc_init(heap, heap + sizeof(heap));
        mp_init();

        mp_obj_list_init(MP_OBJ_TO_PTR(mp_sys_path), 0);
        mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR_));
        mp_obj_list_init(MP_OBJ_TO_PTR(mp_sys_argv), 0);

        if (pyexec_mode_kind == PYEXEC_MODE_FRIENDLY_REPL) {
            const char *main_py = "main.py";
            if (mp_import_stat(main_py) == MP_IMPORT_STAT_FILE) {
                // exec("main.py")
                microbit_pyexec_file(main_py);
            } else {
                // from microbit import *
                mp_import_all(mp_import_name(MP_QSTR_microbit, mp_const_empty_tuple, MP_OBJ_NEW_SMALL_INT(0)));
            }
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
        microbit_soft_timer_deinit();
        gc_sweep_all();
        mp_deinit();
    }
}

STATIC void microbit_display_exception(mp_obj_t exc_in) {
    // Construct the message string ready for display.
    mp_uint_t n, *values;
    mp_obj_exception_get_traceback(exc_in, &n, &values);
    vstr_t vstr;
    mp_print_t print;
    vstr_init_print(&vstr, 50, &print);
    #if MICROPY_ENABLE_SOURCE_LINE
    if (n >= 3) {
        mp_printf(&print, "line %u ", values[1]);
    }
    #endif
    if (mp_obj_is_native_exception_instance(exc_in)) {
        mp_obj_exception_t *exc = MP_OBJ_TO_PTR(exc_in);
        mp_printf(&print, "%q ", exc->base.type->name);
        if (exc->args != NULL && exc->args->len != 0) {
            mp_obj_print_helper(&print, exc->args->items[0], PRINT_STR);
        }
    }

    // Show the message, and allow ctrl-C to stop it.
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_hal_set_interrupt_char(CHAR_CTRL_C);
        microbit_display_show((void *)&microbit_const_image_sad_obj);
        mp_hal_delay_ms(1000);
        microbit_display_scroll(vstr_null_terminated_str(&vstr));
        nlr_pop();
    } else {
        // Uncaught exception, just ignore it.
    }
    mp_hal_set_interrupt_char(-1); // disable interrupt
    mp_handle_pending(false); // clear any pending exceptions (and run any callbacks)
    vstr_clear(&vstr);
}

void microbit_pyexec_file(const char *filename) {
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        // Parse and comple the file.
        mp_lexer_t *lex = mp_lexer_new_from_file(filename);
        qstr source_name = lex->source_name;
        mp_parse_tree_t parse_tree = mp_parse(lex, MP_PARSE_FILE_INPUT);
        mp_obj_t module_fun = mp_compile(&parse_tree, source_name, false);

        // Execute the code.
        mp_hal_set_interrupt_char(CHAR_CTRL_C); // allow ctrl-C to interrupt us
        microbit_outer_nlr_will_handle_soft_timer_exceptions = true;
        mp_call_function_0(module_fun);
        microbit_outer_nlr_will_handle_soft_timer_exceptions = false;
        mp_hal_set_interrupt_char(-1); // disable interrupt
        microbit_soft_timer_deinit(); // stop any background timers
        mp_handle_pending(true); // handle any pending exceptions (and any callbacks)
        nlr_pop();
    } else {
        // Handle uncaught exception.
        microbit_outer_nlr_will_handle_soft_timer_exceptions = false;
        mp_hal_set_interrupt_char(-1); // disable interrupt
        microbit_soft_timer_deinit(); // stop any background timers
        mp_handle_pending(false); // clear any pending exceptions (and run any callbacks)

        mp_obj_t exc_obj = MP_OBJ_FROM_PTR(nlr.ret_val);
        mp_const_obj_t exc_type = MP_OBJ_FROM_PTR(((mp_obj_base_t *)nlr.ret_val)->type);

        // Check if the exception was raised from a run_every callback, and if so
        // use the exception raised by the run_every as the one to display here.
        if (exc_type == &mp_type_SystemExit) {
            mp_obj_exception_t *exc = MP_OBJ_TO_PTR(exc_obj);
            if (exc->args->len == 2
                && exc->args->items[0] == mp_const_none
                && mp_obj_is_exception_instance(exc->args->items[1])) {
                // Extract run_every exception from second entry in args tuple.
                exc_obj = exc->args->items[1];
                exc_type = mp_obj_get_type(exc_obj);
            }
        }

        if (!mp_obj_is_subclass_fast(exc_type, MP_OBJ_FROM_PTR(&mp_type_SystemExit))) {
            // Print exception to stdout.
            mp_obj_print_exception(&mp_plat_print, exc_obj);

            // Print exception to the display, but not if it's KeyboardInterrupt.
            if (!mp_obj_is_subclass_fast(exc_type, MP_OBJ_FROM_PTR(&mp_type_KeyboardInterrupt))) {
                microbit_display_exception(exc_obj);
            }
        }
    }
}

#if MICROPY_MBFS
mp_lexer_t *mp_lexer_new_from_file(const char *filename) {
    return uos_mbfs_new_reader(filename);
}

mp_import_stat_t mp_import_stat(const char *path) {
    return uos_mbfs_import_stat(path);
}

mp_obj_t mp_builtin_open(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    return uos_mbfs_open(n_args, args);
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_builtin_open);
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
