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

#include "py/obj.h"
#include "py/mphal.h"
#include "modmicrobit.h"

STATIC mp_obj_t microbit_reset_(void) {
    microbit_hal_reset();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(microbit_reset_obj, microbit_reset_);

STATIC mp_obj_t microbit_sleep(mp_obj_t ms_in) {
    mp_int_t ms;
    if (mp_obj_is_integer(ms_in)) {
        ms = mp_obj_get_int(ms_in);
    } else {
        ms = (mp_int_t)mp_obj_get_float(ms_in);
    }
    if (ms > 0) {
        mp_hal_delay_ms(ms);
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(microbit_sleep_obj, microbit_sleep);

STATIC mp_obj_t microbit_running_time(void) {
    return MP_OBJ_NEW_SMALL_INT(mp_hal_ticks_ms());
}
MP_DEFINE_CONST_FUN_OBJ_0(microbit_running_time_obj, microbit_running_time);

STATIC mp_obj_t microbit_panic(mp_uint_t n_args, const mp_obj_t *args) {
    if (n_args == 0) {
        // TODO the docs don't mention this, so maybe remove it?
        microbit_hal_panic(999);
    } else {
        microbit_hal_panic(mp_obj_get_int(args[0]));
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(microbit_panic_obj, 0, 1, microbit_panic);

STATIC mp_obj_t microbit_temperature(void) {
    return mp_obj_new_int(microbit_hal_temperature());
}
MP_DEFINE_CONST_FUN_OBJ_0(microbit_temperature_obj, microbit_temperature);

STATIC const mp_rom_map_elem_t microbit_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_microbit) },

    { MP_ROM_QSTR(MP_QSTR_Image), (mp_obj_t)&microbit_image_type },

    { MP_ROM_QSTR(MP_QSTR_display), MP_ROM_PTR(&microbit_display_obj) },
    { MP_ROM_QSTR(MP_QSTR_button_a), (mp_obj_t)&microbit_button_a_obj },
    { MP_ROM_QSTR(MP_QSTR_button_b), (mp_obj_t)&microbit_button_b_obj },
    { MP_ROM_QSTR(MP_QSTR_accelerometer), MP_ROM_PTR(&microbit_accelerometer_obj) },
    { MP_ROM_QSTR(MP_QSTR_compass), MP_ROM_PTR(&microbit_compass_obj) },
    { MP_ROM_QSTR(MP_QSTR_i2c), MP_ROM_PTR(&microbit_i2c_obj) },
    { MP_ROM_QSTR(MP_QSTR_uart), MP_ROM_PTR(&microbit_uart_obj) },
    { MP_ROM_QSTR(MP_QSTR_spi), MP_ROM_PTR(&microbit_spi_obj) },

    { MP_ROM_QSTR(MP_QSTR_reset), MP_ROM_PTR(&microbit_reset_obj) },
    { MP_ROM_QSTR(MP_QSTR_sleep), MP_ROM_PTR(&microbit_sleep_obj) },
    { MP_ROM_QSTR(MP_QSTR_running_time), MP_ROM_PTR(&microbit_running_time_obj) },
    { MP_ROM_QSTR(MP_QSTR_panic), MP_ROM_PTR(&microbit_panic_obj) },
    { MP_ROM_QSTR(MP_QSTR_temperature), MP_ROM_PTR(&microbit_temperature_obj) },

    { MP_ROM_QSTR(MP_QSTR_pin0), MP_ROM_PTR(&microbit_p0_obj) },
    { MP_ROM_QSTR(MP_QSTR_pin1), MP_ROM_PTR(&microbit_p1_obj) },
    { MP_ROM_QSTR(MP_QSTR_pin2), MP_ROM_PTR(&microbit_p2_obj) },
    { MP_ROM_QSTR(MP_QSTR_pin3), MP_ROM_PTR(&microbit_p3_obj) },
    { MP_ROM_QSTR(MP_QSTR_pin4), MP_ROM_PTR(&microbit_p4_obj) },
    { MP_ROM_QSTR(MP_QSTR_pin5), MP_ROM_PTR(&microbit_p5_obj) },
    { MP_ROM_QSTR(MP_QSTR_pin6), MP_ROM_PTR(&microbit_p6_obj) },
    { MP_ROM_QSTR(MP_QSTR_pin7), MP_ROM_PTR(&microbit_p7_obj) },
    { MP_ROM_QSTR(MP_QSTR_pin8), MP_ROM_PTR(&microbit_p8_obj) },
    { MP_ROM_QSTR(MP_QSTR_pin9), MP_ROM_PTR(&microbit_p9_obj) },
    { MP_ROM_QSTR(MP_QSTR_pin10), MP_ROM_PTR(&microbit_p10_obj) },
    { MP_ROM_QSTR(MP_QSTR_pin11), MP_ROM_PTR(&microbit_p11_obj) },
    { MP_ROM_QSTR(MP_QSTR_pin12), MP_ROM_PTR(&microbit_p12_obj) },
    { MP_ROM_QSTR(MP_QSTR_pin13), MP_ROM_PTR(&microbit_p13_obj) },
    { MP_ROM_QSTR(MP_QSTR_pin14), MP_ROM_PTR(&microbit_p14_obj) },
    { MP_ROM_QSTR(MP_QSTR_pin15), MP_ROM_PTR(&microbit_p15_obj) },
    { MP_ROM_QSTR(MP_QSTR_pin16), MP_ROM_PTR(&microbit_p16_obj) },
    { MP_ROM_QSTR(MP_QSTR_pin19), MP_ROM_PTR(&microbit_p19_obj) },
    { MP_ROM_QSTR(MP_QSTR_pin20), MP_ROM_PTR(&microbit_p20_obj) },
    { MP_ROM_QSTR(MP_QSTR_pin_speaker), MP_ROM_PTR(&microbit_pin_speaker_obj) },
};

STATIC MP_DEFINE_CONST_DICT(microbit_module_globals, microbit_module_globals_table);

const mp_obj_module_t microbit_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&microbit_module_globals,
};
