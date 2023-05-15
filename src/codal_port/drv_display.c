/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015-2020 Damien P. George
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

#include <string.h>
#include "py/runtime.h"
#include "py/gc.h"
#include "py/objstr.h"
#include "py/mphal.h"
#include "drv_display.h"

#define MILLISECONDS_PER_MACRO_TICK 6

#define ASYNC_MODE_STOPPED 0
#define ASYNC_MODE_ANIMATION 1
#define ASYNC_MODE_CLEAR 2

static uint8_t async_mode;
static mp_obj_t async_iterator = NULL;
static volatile bool wakeup_event = false;
static mp_uint_t async_delay = 1000;
static mp_uint_t async_tick = 0;
static bool async_clear = false;

STATIC void async_stop(void) {
    async_iterator = NULL;
    async_mode = ASYNC_MODE_STOPPED;
    async_tick = 0;
    async_delay = 1000;
    async_clear = false;
    MP_STATE_PORT(display_data) = NULL;
    wakeup_event = true;
}

void microbit_display_init(void) {
    async_stop();
}

void microbit_display_stop(void) {
    MP_STATE_PORT(display_data) = NULL;
}

STATIC void wait_for_event() {
    while (!wakeup_event) {
        // allow CTRL-C to stop the animation
        if (MP_STATE_THREAD(mp_pending_exception) != MP_OBJ_NULL) {
            async_stop();
            mp_handle_pending(true);
            return;
        }
        microbit_hal_idle();
    }
    wakeup_event = false;
}

static void draw_object(mp_obj_t obj) {
    if (obj == MP_OBJ_STOP_ITERATION) {
        if (async_clear) {
            microbit_display_show(BLANK_IMAGE);
            async_clear = false;
        } else {
            async_stop();
        }
    } else if (mp_obj_get_type(obj) == &microbit_image_type) {
        microbit_display_show((microbit_image_obj_t *)obj);
    } else if (MP_OBJ_IS_STR(obj)) {
        mp_uint_t len;
        const char *str = mp_obj_str_get_data(obj, &len);
        if (len == 1) {
            microbit_display_show(microbit_image_for_char(str[0]));
        } else {
            async_stop();
        }
    } else {
        mp_sched_exception(mp_obj_new_exception_msg(&mp_type_TypeError, MP_ERROR_TEXT("not an image")));
        async_stop();
    }
}

// TODO: pass in current timestamp as arg
void microbit_display_update(void) {
    async_tick += MILLISECONDS_PER_MACRO_TICK;
    if (async_tick < async_delay) {
        return;
    }
    async_tick = 0;
    switch (async_mode) {
        case ASYNC_MODE_ANIMATION:
        {
            if (MP_STATE_PORT(display_data) == NULL) {
                async_stop();
                break;
            }
            mp_obj_t obj;
            nlr_buf_t nlr;
            gc_lock();
            if (nlr_push(&nlr) == 0) {
                obj = mp_iternext_allow_raise(async_iterator);
                nlr_pop();
                gc_unlock();
            } else {
                gc_unlock();
                if (!mp_obj_is_subclass_fast(MP_OBJ_FROM_PTR(((mp_obj_base_t*)nlr.ret_val)->type),
                    MP_OBJ_FROM_PTR(&mp_type_StopIteration))) {
                    if (mp_obj_get_type(nlr.ret_val) == &mp_type_MemoryError) {
                        mp_printf(&mp_plat_print, "Allocation in interrupt handler");
                    }
                    mp_sched_exception(MP_OBJ_FROM_PTR(nlr.ret_val));
                }
                obj = MP_OBJ_STOP_ITERATION;
            }
            draw_object(obj);
            break;
        }
        case ASYNC_MODE_CLEAR:
            microbit_display_show(BLANK_IMAGE);
            async_stop();
            break;
    }
}

void microbit_display_clear(void) {
    // Reset repeat state, cancel animation and clear screen.
    // The actual screen clearing will be done by microbit_display_update.
    wakeup_event = false;
    async_mode = ASYNC_MODE_CLEAR;
    async_tick = async_delay - MILLISECONDS_PER_MACRO_TICK;
    wait_for_event();
}

void microbit_display_show(microbit_image_obj_t *image) {
    mp_int_t w = MIN(image_width(image), 5);
    mp_int_t h = MIN(image_height(image), 5);
    mp_int_t x = 0;
    for (; x < w; ++x) {
        mp_int_t y = 0;
        for (; y < h; ++y) {
            uint8_t pix = image_get_pixel(image, x, y);
            microbit_hal_display_set_pixel(x, y, pix);
        }
        for (; y < 5; ++y) {
            microbit_hal_display_set_pixel(x, y, 0);
        }
    }
    for (; x < 5; ++x) {
        for (mp_int_t y = 0; y < 5; ++y) {
            microbit_hal_display_set_pixel(x, y, 0);
        }
    }
}

void microbit_display_scroll(const char *str) {
    mp_obj_t iterable = scrolling_string_image_iterable(str, strlen(str), NULL, false, false);
    microbit_display_animate(iterable, DEFAULT_SCROLL_SPEED_MS, false, true);
}

void microbit_display_animate(mp_obj_t iterable, mp_int_t delay, bool clear, bool wait) {
    // Reset the repeat state.
    MP_STATE_PORT(display_data) = NULL;
    async_iterator = mp_getiter(iterable, NULL);
    async_delay = delay;
    async_clear = clear;
    MP_STATE_PORT(display_data) = async_iterator;
    wakeup_event = false;
    mp_obj_t obj = mp_iternext_allow_raise(async_iterator);
    draw_object(obj);
    async_tick = 0;
    async_mode = ASYNC_MODE_ANIMATION;
    if (wait) {
        wait_for_event();
    }
}

MP_REGISTER_ROOT_POINTER(void *display_data);
