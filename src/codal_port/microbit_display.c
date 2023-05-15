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

#include "py/runtime.h"
#include "py/objstr.h"
#include "py/mphal.h"
#include "iters.h"
#include "modmicrobit.h"

#define DEFAULT_PRINT_SPEED_MS 400

typedef struct _microbit_display_obj_t {
    mp_obj_base_t base;
    bool active;
} microbit_display_obj_t;

mp_obj_t microbit_display_show_func(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_image, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_delay, MP_ARG_INT, {.u_int = DEFAULT_PRINT_SPEED_MS} },
        { MP_QSTR_clear, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_wait, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = true} },
        { MP_QSTR_loop, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
    };

    // Parse the args.
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_obj_t image = args[0].u_obj;
    mp_int_t delay = args[1].u_int;
    bool clear = args[2].u_bool;
    bool wait = args[3].u_bool;
    bool loop = args[4].u_bool;

    // Cancel any animations.
    microbit_display_stop();

    // Convert to string from an integer or float if applicable
    if (mp_obj_is_integer(image) || mp_obj_is_float(image)) {
        image = mp_obj_str_make_new(&mp_type_str, 1, 0, &image);
    }

    if (MP_OBJ_IS_STR(image)) {
        // arg is a string object
        mp_uint_t len;
        const char *str = mp_obj_str_get_data(image, &len);
        if (len == 0) {
            // There are no chars; do nothing.
            return mp_const_none;
        } else if (len == 1) {
            if (!clear && !loop) {
                // A single char; convert to an image and print that.
                image = microbit_image_for_char(str[0]);
                goto single_image_immediate;
            }
        }
        image = microbit_string_facade(image);
    } else if (mp_obj_get_type(image) == &microbit_image_type) {
        if (!clear && !loop) {
            goto single_image_immediate;
        }
        image = mp_obj_new_tuple(1, &image);
    }

    if (args[4].u_bool) { /*loop*/
        image = microbit_repeat_iterator(image);
    }
    microbit_display_animate(image, delay, clear, wait);
    return mp_const_none;

single_image_immediate:
    microbit_display_show((microbit_image_obj_t *)image);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(microbit_display_show_obj, 1, microbit_display_show_func);

mp_obj_t microbit_display_scroll_func(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_text, ARG_delay, ARG_wait, ARG_monospace, ARG_loop };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_text, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_delay, MP_ARG_INT, {.u_int = DEFAULT_SCROLL_SPEED_MS} },
        { MP_QSTR_wait, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = true} },
        { MP_QSTR_monospace, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
        { MP_QSTR_loop, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
    };
    // Parse the args.
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    mp_uint_t len;
    mp_obj_t object_string = args[0].u_obj;
    if (mp_obj_is_integer(object_string) || mp_obj_is_float(object_string)) {
        object_string = mp_obj_str_make_new(&mp_type_str, 1, 0, &object_string);
    }
    const char* str = mp_obj_str_get_data(object_string, &len);
    mp_obj_t iterable = scrolling_string_image_iterable(str, len, args[0].u_obj, args[3].u_bool /*monospace?*/, args[4].u_bool /*loop*/);
    microbit_display_animate(iterable, args[1].u_int /*delay*/, false/*clear*/, args[2].u_bool/*wait?*/);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(microbit_display_scroll_obj, 1, microbit_display_scroll_func);

mp_obj_t microbit_display_on_func(mp_obj_t self_in) {
    microbit_display_obj_t *self = MP_OBJ_TO_PTR(self_in);
    microbit_obj_pin_acquire(&microbit_p3_obj, microbit_pin_mode_display);
    microbit_obj_pin_acquire(&microbit_p4_obj, microbit_pin_mode_display);
    microbit_obj_pin_acquire(&microbit_p6_obj, microbit_pin_mode_display);
    microbit_obj_pin_acquire(&microbit_p7_obj, microbit_pin_mode_display);
    microbit_obj_pin_acquire(&microbit_p10_obj, microbit_pin_mode_display);
    microbit_display_init();
    self->active = true;
    microbit_hal_display_enable(1);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_display_on_obj, microbit_display_on_func);

mp_obj_t microbit_display_off_func(mp_obj_t self_in) {
    microbit_display_obj_t *self = MP_OBJ_TO_PTR(self_in);
    microbit_hal_display_enable(0);
    self->active = false;
    microbit_obj_pin_free(&microbit_p3_obj);
    microbit_obj_pin_free(&microbit_p4_obj);
    microbit_obj_pin_free(&microbit_p6_obj);
    microbit_obj_pin_free(&microbit_p7_obj);
    microbit_obj_pin_free(&microbit_p10_obj);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_display_off_obj, microbit_display_off_func);

mp_obj_t microbit_display_is_on_func(mp_obj_t obj) {
    microbit_display_obj_t *self = (microbit_display_obj_t*)obj;
    return mp_obj_new_bool(self->active);
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_display_is_on_obj, microbit_display_is_on_func);

mp_obj_t microbit_display_read_light_level(mp_obj_t obj) {
    (void)obj;
    return MP_OBJ_NEW_SMALL_INT(microbit_hal_display_read_light_level());
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_display_read_light_level_obj, microbit_display_read_light_level);

mp_obj_t microbit_display_clear_func(mp_obj_t self) {
    (void)self;
    microbit_display_clear();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_display_clear_obj, microbit_display_clear_func);

void microbit_display_set_pixel(microbit_display_obj_t *display, mp_int_t x, mp_int_t y, mp_int_t bright) {
    if (x < 0 || y < 0 || x >= MICROBIT_DISPLAY_WIDTH || y >= MICROBIT_DISPLAY_HEIGHT) {
        mp_raise_ValueError(MP_ERROR_TEXT("index out of bounds"));
    }
    if (bright < 0 || bright > MICROBIT_DISPLAY_MAX_BRIGHTNESS) {
        mp_raise_ValueError(MP_ERROR_TEXT("brightness out of bounds"));
    }
    microbit_hal_display_set_pixel(x, y, bright);
}

STATIC mp_obj_t microbit_display_set_pixel_func(mp_uint_t n_args, const mp_obj_t *args) {
    (void)n_args;
    microbit_display_obj_t *self = (microbit_display_obj_t*)args[0];
    microbit_display_set_pixel(self, mp_obj_get_int(args[1]), mp_obj_get_int(args[2]), mp_obj_get_int(args[3]));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(microbit_display_set_pixel_obj, 4, 4, microbit_display_set_pixel_func);

mp_int_t microbit_display_get_pixel(microbit_display_obj_t *display, mp_int_t x, mp_int_t y) {
    if (x < 0 || y < 0 || x >= MICROBIT_DISPLAY_WIDTH || y >= MICROBIT_DISPLAY_HEIGHT) {
        mp_raise_ValueError(MP_ERROR_TEXT("index out of bounds"));
    }
    return microbit_hal_display_get_pixel(x, y);
}

STATIC mp_obj_t microbit_display_get_pixel_func(mp_obj_t self_in, mp_obj_t x_in, mp_obj_t y_in) {
    microbit_display_obj_t *self = (microbit_display_obj_t*)self_in;
    return MP_OBJ_NEW_SMALL_INT(microbit_display_get_pixel(self, mp_obj_get_int(x_in), mp_obj_get_int(y_in)));
}
MP_DEFINE_CONST_FUN_OBJ_3(microbit_display_get_pixel_obj, microbit_display_get_pixel_func);

STATIC const mp_rom_map_elem_t microbit_display_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_get_pixel), MP_ROM_PTR(&microbit_display_get_pixel_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_pixel), MP_ROM_PTR(&microbit_display_set_pixel_obj) },
    { MP_ROM_QSTR(MP_QSTR_show), MP_ROM_PTR(&microbit_display_show_obj) },
    { MP_ROM_QSTR(MP_QSTR_scroll), MP_ROM_PTR(&microbit_display_scroll_obj) },
    { MP_ROM_QSTR(MP_QSTR_clear), MP_ROM_PTR(&microbit_display_clear_obj) },
    { MP_ROM_QSTR(MP_QSTR_on), MP_ROM_PTR(&microbit_display_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_off), MP_ROM_PTR(&microbit_display_off_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_on), MP_ROM_PTR(&microbit_display_is_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_read_light_level),MP_ROM_PTR(&microbit_display_read_light_level_obj) },
};
STATIC MP_DEFINE_CONST_DICT(microbit_display_locals_dict, microbit_display_locals_dict_table);

STATIC MP_DEFINE_CONST_OBJ_TYPE(
    microbit_display_type,
    MP_QSTR_MicroBitDisplay,
    MP_TYPE_FLAG_NONE,
    locals_dict, &microbit_display_locals_dict
    );

microbit_display_obj_t microbit_display_obj = {
    .base = {&microbit_display_type},
    .active = true,
};
