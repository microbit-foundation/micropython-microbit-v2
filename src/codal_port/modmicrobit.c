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

#include <math.h>
#include "py/obj.h"
#include "py/mphal.h"
#include "drv_softtimer.h"
#include "drv_system.h"
#include "modaudio.h"
#include "modmicrobit.h"

STATIC mp_obj_t microbit_run_every_new(uint32_t period_ms);

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

STATIC mp_obj_t microbit_set_volume(mp_obj_t volume_in) {
    mp_int_t volume = mp_obj_get_int(volume_in);
    if (volume < 0) {
        volume = 0;
    } else if (volume > 255) {
        volume = 255;
    }
    microbit_hal_audio_set_volume(volume);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(microbit_set_volume_obj, microbit_set_volume);

STATIC mp_obj_t microbit_ws2812_write(mp_obj_t pin_in, mp_obj_t buf_in) {
    uint8_t pin = microbit_obj_get_pin(pin_in)->name;
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_READ);
    microbit_hal_pin_write_ws2812(pin, bufinfo.buf, bufinfo.len);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(microbit_ws2812_write_obj, microbit_ws2812_write);

STATIC mp_obj_t microbit_run_every(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_callback, ARG_days, ARG_h, ARG_min, ARG_s, ARG_ms };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_callback, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_days, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_h, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_min, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_s, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_ms, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint32_t period_ms = args[ARG_days].u_int * 24 * 60 * 60 * 1000
        + args[ARG_h].u_int * 60 * 60 * 1000
        + args[ARG_min].u_int * 60 * 1000
        + args[ARG_s].u_int * 1000
        + args[ARG_ms].u_int;

    mp_obj_t run_every = microbit_run_every_new(period_ms);

    if (args[ARG_callback].u_obj == mp_const_none) {
        // Return decorator-compatible object.
        return run_every;
    } else {
        // Start the timer now.
        return MP_OBJ_TYPE_GET_SLOT(mp_obj_get_type(run_every), call)(run_every, 1, 0, &args[ARG_callback].u_obj);
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(microbit_run_every_obj, 0, microbit_run_every);

STATIC mp_obj_t microbit_scale(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_value, ARG_from_, ARG_to };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_value, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_from_, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_to, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // Extract from/to min/max arrays.
    mp_obj_t *from_items, *to_items;
    mp_obj_get_array_fixed_n(args[ARG_from_].u_obj, 2, &from_items);
    mp_obj_get_array_fixed_n(args[ARG_to].u_obj, 2, &to_items);

    // Extract all float values.
    mp_float_t from_value = mp_obj_get_float(args[ARG_value].u_obj);
    mp_float_t from_min = mp_obj_get_float(from_items[0]);
    mp_float_t from_max = mp_obj_get_float(from_items[1]);
    mp_float_t to_min = mp_obj_get_float(to_items[0]);
    mp_float_t to_max = mp_obj_get_float(to_items[1]);

    // Compute scaled value.
    mp_float_t to_value = (from_value - from_min) / (from_max - from_min) * (to_max - to_min) + to_min;

    // Return float or int based on type of "to" tuple.
    if (mp_obj_is_float(to_items[0]) || mp_obj_is_float(to_items[1])) {
        return mp_obj_new_float(to_value);
    } else {
        return mp_obj_new_int(MICROPY_FLOAT_C_FUN(nearbyint)(to_value));
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(microbit_scale_obj, 0, microbit_scale);

STATIC const mp_rom_map_elem_t microbit_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_microbit) },

    { MP_ROM_QSTR(MP_QSTR_Image), (mp_obj_t)&microbit_image_type },
    { MP_ROM_QSTR(MP_QSTR_Sound), MP_ROM_PTR(&microbit_sound_type) },
    { MP_ROM_QSTR(MP_QSTR_SoundEvent), (mp_obj_t)&microbit_soundevent_type },

    { MP_ROM_QSTR(MP_QSTR_display), MP_ROM_PTR(&microbit_display_obj) },
    { MP_ROM_QSTR(MP_QSTR_button_a), (mp_obj_t)&microbit_button_a_obj },
    { MP_ROM_QSTR(MP_QSTR_button_b), (mp_obj_t)&microbit_button_b_obj },
    { MP_ROM_QSTR(MP_QSTR_accelerometer), MP_ROM_PTR(&microbit_accelerometer_obj) },
    { MP_ROM_QSTR(MP_QSTR_compass), MP_ROM_PTR(&microbit_compass_obj) },
    { MP_ROM_QSTR(MP_QSTR_speaker), MP_ROM_PTR(&microbit_speaker_obj) },
    { MP_ROM_QSTR(MP_QSTR_microphone), MP_ROM_PTR(&microbit_microphone_obj) },
    { MP_ROM_QSTR(MP_QSTR_audio), MP_ROM_PTR(&audio_module) },

    { MP_ROM_QSTR(MP_QSTR_i2c), MP_ROM_PTR(&microbit_i2c_obj) },
    { MP_ROM_QSTR(MP_QSTR_uart), MP_ROM_PTR(&microbit_uart_obj) },
    { MP_ROM_QSTR(MP_QSTR_spi), MP_ROM_PTR(&microbit_spi_obj) },

    { MP_ROM_QSTR(MP_QSTR_reset), MP_ROM_PTR(&microbit_reset_obj) },
    { MP_ROM_QSTR(MP_QSTR_sleep), MP_ROM_PTR(&microbit_sleep_obj) },
    { MP_ROM_QSTR(MP_QSTR_running_time), MP_ROM_PTR(&microbit_running_time_obj) },
    { MP_ROM_QSTR(MP_QSTR_panic), MP_ROM_PTR(&microbit_panic_obj) },
    { MP_ROM_QSTR(MP_QSTR_temperature), MP_ROM_PTR(&microbit_temperature_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_volume), MP_ROM_PTR(&microbit_set_volume_obj) },
    { MP_ROM_QSTR(MP_QSTR_ws2812_write), MP_ROM_PTR(&microbit_ws2812_write_obj) },

    { MP_ROM_QSTR(MP_QSTR_run_every), MP_ROM_PTR(&microbit_run_every_obj) },
    { MP_ROM_QSTR(MP_QSTR_scale), MP_ROM_PTR(&microbit_scale_obj) },

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
    { MP_ROM_QSTR(MP_QSTR_pin_logo), MP_ROM_PTR(&microbit_pin_logo_obj) },
    { MP_ROM_QSTR(MP_QSTR_pin_speaker), MP_ROM_PTR(&microbit_pin_speaker_obj) },
};

STATIC MP_DEFINE_CONST_DICT(microbit_module_globals, microbit_module_globals_table);

const mp_obj_module_t microbit_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&microbit_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_microbit, microbit_module);

/******************************************************************************/
// run_every object

typedef struct _microbit_run_every_obj_t {
    microbit_soft_timer_entry_t timer;
    mp_obj_t user_callback;
} microbit_run_every_obj_t;

STATIC mp_obj_t microbit_run_every_callback(mp_obj_t self_in) {
    microbit_run_every_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (self->user_callback == MP_OBJ_NULL) {
        // Callback is disabled.
        return mp_const_none;
    }

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_call_function_0(self->user_callback);
        nlr_pop();
    } else {
        // Exception raise, so stope this callback from being called again.
        self->timer.mode = MICROBIT_SOFT_TIMER_MODE_ONE_SHOT;
        self->user_callback = MP_OBJ_NULL;

        mp_obj_t exc = MP_OBJ_FROM_PTR(nlr.ret_val);
        if (microbit_outer_nlr_will_handle_soft_timer_exceptions) {
            // The outer NLR handler will handle this exception so raise it via a SystemExit.
            mp_obj_t args[2] = {mp_const_none, exc};
            mp_sched_exception(mp_obj_exception_make_new(&mp_type_SystemExit, 2, 0, args));
        } else {
            // Print exception to stdout right now.
            mp_obj_print_exception(&mp_plat_print, exc);
        }
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(microbit_run_every_callback_obj, microbit_run_every_callback);

STATIC mp_obj_t microbit_run_every_obj_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    microbit_run_every_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_arg_check_num(n_args, n_kw, 1, 1, false);
    self->timer.py_callback = MP_OBJ_FROM_PTR(&microbit_run_every_callback_obj);
    self->user_callback = args[0];
    microbit_soft_timer_insert(&self->timer, self->timer.delta_ms);
    return self_in;
}

STATIC MP_DEFINE_CONST_OBJ_TYPE(
    microbit_run_every_obj_type,
    MP_QSTR_run_every,
    MP_TYPE_FLAG_NONE,
    call, microbit_run_every_obj_call
    );

STATIC mp_obj_t microbit_run_every_new(uint32_t period_ms) {
    microbit_run_every_obj_t *self = m_new_obj(microbit_run_every_obj_t);
    self->timer.pairheap.base.type = &microbit_run_every_obj_type;
    self->timer.flags = MICROBIT_SOFT_TIMER_FLAG_PY_CALLBACK | MICROBIT_SOFT_TIMER_FLAG_GC_ALLOCATED;
    self->timer.mode = MICROBIT_SOFT_TIMER_MODE_PERIODIC;
    self->timer.delta_ms = period_ms;
    self->user_callback = MP_OBJ_NULL;
    return MP_OBJ_FROM_PTR(self);
}
