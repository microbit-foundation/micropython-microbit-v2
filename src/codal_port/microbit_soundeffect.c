/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Damien P. George
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
#include "modmicrobit.h"
#include "modaudio.h"

#define SOUND_EXPR_WAVEFORM_OFFSET              (0)
#define SOUND_EXPR_WAVEFORM_LENGTH              (1)
#define SOUND_EXPR_VOLUME_START_OFFSET          (1)
#define SOUND_EXPR_VOLUME_START_LENGTH          (4)
#define SOUND_EXPR_FREQUENCY_START_OFFSET       (5)
#define SOUND_EXPR_FREQUENCY_START_LENGTH       (4)
#define SOUND_EXPR_DURATION_OFFSET              (9)
#define SOUND_EXPR_DURATION_LENGTH              (4)
#define SOUND_EXPR_SHAPE_OFFSET                 (13)
#define SOUND_EXPR_SHAPE_LENGTH                 (2)
#define SOUND_EXPR_FREQUENCY_END_OFFSET         (18)
#define SOUND_EXPR_FREQUENCY_END_LENGTH         (4)
#define SOUND_EXPR_VOLUME_END_OFFSET            (26)
#define SOUND_EXPR_VOLUME_END_LENGTH            (4)
#define SOUND_EXPR_STEPS_OFFSET                 (30)
#define SOUND_EXPR_STEPS_LENGTH                 (4)
#define SOUND_EXPR_FX_CHOICE_OFFSET             (34)
#define SOUND_EXPR_FX_CHOICE_LENGTH             (2)
#define SOUND_EXPR_FX_PARAM_OFFSET              (36)
#define SOUND_EXPR_FX_PARAM_LENGTH              (4)
#define SOUND_EXPR_FX_STEPS_OFFSET              (40)
#define SOUND_EXPR_FX_STEPS_LENGTH              (4)

#define SOUND_EXPR_ENCODE_VOLUME(v)             (((v) * 1023 + 127) / 255)
#define SOUND_EXPR_DECODE_VOLUME(v)             (((v) * 255 + 511) / 1023)

#define SOUND_EFFECT_WAVEFORM_SINE              (0)
#define SOUND_EFFECT_WAVEFORM_SAWTOOTH          (1)
#define SOUND_EFFECT_WAVEFORM_TRIANGLE          (2)
#define SOUND_EFFECT_WAVEFORM_SQUARE            (3)
#define SOUND_EFFECT_WAVEFORM_NOISE             (4)

#define SOUND_EFFECT_SHAPE_LINEAR               (1)
#define SOUND_EFFECT_SHAPE_CURVE                (2)
#define SOUND_EFFECT_SHAPE_LOG                  (18)

#define SOUND_EFFECT_FX_NONE                    (0)
#define SOUND_EFFECT_FX_TREMOLO                 (2)
#define SOUND_EFFECT_FX_VIBRATO                 (1)
#define SOUND_EFFECT_FX_WARBLE                  (3)

#define SOUND_EFFECT_DEFAULT_FREQ_START         (500)
#define SOUND_EFFECT_DEFAULT_FREQ_END           (2500)
#define SOUND_EFFECT_DEFAULT_DURATION           (500)
#define SOUND_EFFECT_DEFAULT_VOL_START          (255)
#define SOUND_EFFECT_DEFAULT_VOL_END            (0)
#define SOUND_EFFECT_DEFAULT_WAVEFORM           (SOUND_EFFECT_WAVEFORM_SQUARE)
#define SOUND_EFFECT_DEFAULT_FX                 (SOUND_EFFECT_FX_NONE)
#define SOUND_EFFECT_DEFAULT_SHAPE              (SOUND_EFFECT_SHAPE_LOG)

typedef struct _microbit_soundeffect_obj_t {
    mp_obj_base_t base;
    bool is_mutable;
    char sound_expr[SOUND_EXPR_TOTAL_LENGTH];
} microbit_soundeffect_obj_t;

typedef struct _soundeffect_attr_t {
    uint16_t qst;
    uint8_t offset;
    uint8_t length;
} soundeffect_attr_t;

STATIC const uint16_t waveform_to_qstr_table[5] = {
    [SOUND_EFFECT_WAVEFORM_SINE] = MP_QSTR_WAVEFORM_SINE,
    [SOUND_EFFECT_WAVEFORM_SAWTOOTH] = MP_QSTR_WAVEFORM_SAWTOOTH,
    [SOUND_EFFECT_WAVEFORM_TRIANGLE] = MP_QSTR_WAVEFORM_TRIANGLE,
    [SOUND_EFFECT_WAVEFORM_SQUARE] = MP_QSTR_WAVEFORM_SQUARE,
    [SOUND_EFFECT_WAVEFORM_NOISE] = MP_QSTR_WAVEFORM_NOISE,
};

STATIC const uint16_t fx_to_qstr_table[4] = {
    [SOUND_EFFECT_FX_NONE] = MP_QSTR_FX_NONE,
    [SOUND_EFFECT_FX_TREMOLO] = MP_QSTR_FX_TREMOLO,
    [SOUND_EFFECT_FX_VIBRATO] = MP_QSTR_FX_VIBRATO,
    [SOUND_EFFECT_FX_WARBLE] = MP_QSTR_FX_WARBLE,
};

STATIC const soundeffect_attr_t soundeffect_attr_table[] = {
    { MP_QSTR_freq_start, SOUND_EXPR_FREQUENCY_START_OFFSET, SOUND_EXPR_FREQUENCY_START_LENGTH },
    { MP_QSTR_freq_end, SOUND_EXPR_FREQUENCY_END_OFFSET, SOUND_EXPR_FREQUENCY_END_LENGTH },
    { MP_QSTR_duration, SOUND_EXPR_DURATION_OFFSET, SOUND_EXPR_DURATION_LENGTH },
    { MP_QSTR_vol_start, SOUND_EXPR_VOLUME_START_OFFSET, SOUND_EXPR_VOLUME_START_LENGTH },
    { MP_QSTR_vol_end, SOUND_EXPR_VOLUME_END_OFFSET, SOUND_EXPR_VOLUME_END_LENGTH },
    { MP_QSTR_waveform, SOUND_EXPR_WAVEFORM_OFFSET, SOUND_EXPR_WAVEFORM_LENGTH },
    { MP_QSTR_fx, SOUND_EXPR_FX_CHOICE_OFFSET, SOUND_EXPR_FX_CHOICE_LENGTH },
    { MP_QSTR_shape, SOUND_EXPR_SHAPE_OFFSET, SOUND_EXPR_SHAPE_LENGTH },
};

const char *microbit_soundeffect_get_sound_expr_data(mp_obj_t self_in) {
    const microbit_soundeffect_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return &self->sound_expr[0];
}

STATIC void sound_expr_encode(microbit_soundeffect_obj_t *self, size_t offset, size_t length, unsigned int value) {
    if (offset == SOUND_EXPR_VOLUME_START_OFFSET || offset == SOUND_EXPR_VOLUME_END_OFFSET) {
        value = SOUND_EXPR_ENCODE_VOLUME(value);
    }
    for (size_t i = length; i > 0; --i) {
        self->sound_expr[offset + i - 1] = '0' + value % 10;
        value /= 10;
    }
    if (value != 0) {
        if (length == 1) {
            mp_raise_ValueError(MP_ERROR_TEXT("maximum value is 9"));
        } else if (length == 2) {
            mp_raise_ValueError(MP_ERROR_TEXT("maximum value is 99"));
        } else {
            mp_raise_ValueError(MP_ERROR_TEXT("maximum value is 9999"));
        }
    }
}

STATIC unsigned int sound_expr_decode(const microbit_soundeffect_obj_t *self, size_t offset, size_t length) {
    unsigned int value = 0;
    for (size_t i = 0; i < length; ++i) {
        value = value * 10 + self->sound_expr[offset + i] - '0';
    }
    if (offset == SOUND_EXPR_VOLUME_START_OFFSET || offset == SOUND_EXPR_VOLUME_END_OFFSET) {
        value = SOUND_EXPR_DECODE_VOLUME(value);
    }
    return value;
}

STATIC void microbit_soundeffect_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    const microbit_soundeffect_obj_t *self = MP_OBJ_TO_PTR(self_in);

    unsigned int freq_start = sound_expr_decode(self, SOUND_EXPR_FREQUENCY_START_OFFSET, SOUND_EXPR_FREQUENCY_START_LENGTH);
    unsigned int freq_end = sound_expr_decode(self, SOUND_EXPR_FREQUENCY_END_OFFSET, SOUND_EXPR_FREQUENCY_END_LENGTH);
    unsigned int duration = sound_expr_decode(self, SOUND_EXPR_DURATION_OFFSET, SOUND_EXPR_DURATION_LENGTH);
    unsigned int vol_start = sound_expr_decode(self, SOUND_EXPR_VOLUME_START_OFFSET, SOUND_EXPR_VOLUME_START_LENGTH);
    unsigned int vol_end = sound_expr_decode(self, SOUND_EXPR_VOLUME_END_OFFSET, SOUND_EXPR_VOLUME_END_LENGTH);
    unsigned int waveform = sound_expr_decode(self, SOUND_EXPR_WAVEFORM_OFFSET, SOUND_EXPR_WAVEFORM_LENGTH);
    unsigned int fx = sound_expr_decode(self, SOUND_EXPR_FX_CHOICE_OFFSET, SOUND_EXPR_FX_CHOICE_LENGTH);
    unsigned int shape = sound_expr_decode(self, SOUND_EXPR_SHAPE_OFFSET, SOUND_EXPR_SHAPE_LENGTH);

    if (kind == PRINT_STR) {
        mp_printf(print, "SoundEffect("
            "freq_start=%d, "
            "freq_end=%d, "
            "duration=%d, "
            "vol_start=%d, "
            "vol_end=%d, "
            "waveform=%q, "
            "fx=%q, ",
            freq_start,
            freq_end,
            duration,
            vol_start,
            vol_end,
            waveform_to_qstr_table[waveform],
            fx_to_qstr_table[fx]
        );

        // Support shape values that don't have a corresponding constant assigned.
        switch (shape) {
            case SOUND_EFFECT_SHAPE_LINEAR:
                mp_printf(print, "shape=SHAPE_LINEAR)");
                break;
            case SOUND_EFFECT_SHAPE_CURVE:
                mp_printf(print, "shape=SHAPE_CURVE)");
                break;
            case SOUND_EFFECT_SHAPE_LOG:
                mp_printf(print, "shape=SHAPE_LOG)");
                break;
            default:
                mp_printf(print, "shape=%d)", shape);
                break;
        }
    } else {
        // PRINT_REPR
        mp_printf(print, "SoundEffect(%d, %d, %d, %d, %d, %d, %d, %d)",
            freq_start, freq_end, duration, vol_start, vol_end, waveform, fx, shape);
    }
}

// Constructor:
// SoundEffect(freq_start, freq_end, duration, vol_start, vol_end, waveform, fx, shape)
STATIC mp_obj_t microbit_soundeffect_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args_in) {
    enum { ARG_freq_start, ARG_freq_end, ARG_duration, ARG_vol_start, ARG_vol_end, ARG_waveform, ARG_fx, ARG_shape };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_freq_start,       MP_ARG_INT, {.u_int = SOUND_EFFECT_DEFAULT_FREQ_START} },
        { MP_QSTR_freq_end,         MP_ARG_INT, {.u_int = SOUND_EFFECT_DEFAULT_FREQ_END} },
        { MP_QSTR_duration,         MP_ARG_INT, {.u_int = SOUND_EFFECT_DEFAULT_DURATION} },
        { MP_QSTR_vol_start,        MP_ARG_INT, {.u_int = SOUND_EFFECT_DEFAULT_VOL_START} },
        { MP_QSTR_vol_end,          MP_ARG_INT, {.u_int = SOUND_EFFECT_DEFAULT_VOL_END} },
        { MP_QSTR_waveform,         MP_ARG_INT, {.u_int = SOUND_EFFECT_DEFAULT_WAVEFORM} },
        { MP_QSTR_fx,               MP_ARG_INT, {.u_int = SOUND_EFFECT_DEFAULT_FX} },
        { MP_QSTR_shape,            MP_ARG_INT, {.u_int = SOUND_EFFECT_DEFAULT_SHAPE} },
    };

    // Parse arguments.
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, args_in, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // Create sound effect object.
    microbit_soundeffect_obj_t *self = m_new_obj(microbit_soundeffect_obj_t);
    self->base.type = type;
    self->is_mutable = true;

    // Initialise base parameters of the sound expression data.
    memset(&self->sound_expr[0], '0', SOUND_EXPR_TOTAL_LENGTH);
    sound_expr_encode(self, SOUND_EXPR_STEPS_OFFSET, SOUND_EXPR_STEPS_LENGTH, 128);
    sound_expr_encode(self, SOUND_EXPR_FX_PARAM_OFFSET, SOUND_EXPR_FX_PARAM_LENGTH, 1);
    sound_expr_encode(self, SOUND_EXPR_FX_STEPS_OFFSET, SOUND_EXPR_FX_STEPS_LENGTH, 24);

    // Modify any given parameters.
    sound_expr_encode(self, SOUND_EXPR_FREQUENCY_START_OFFSET, SOUND_EXPR_FREQUENCY_START_LENGTH, args[ARG_freq_start].u_int);
    sound_expr_encode(self, SOUND_EXPR_FREQUENCY_END_OFFSET, SOUND_EXPR_FREQUENCY_END_LENGTH, args[ARG_freq_end].u_int);
    sound_expr_encode(self, SOUND_EXPR_DURATION_OFFSET, SOUND_EXPR_DURATION_LENGTH, args[ARG_duration].u_int);
    sound_expr_encode(self, SOUND_EXPR_VOLUME_START_OFFSET, SOUND_EXPR_VOLUME_START_LENGTH, args[ARG_vol_start].u_int);
    sound_expr_encode(self, SOUND_EXPR_VOLUME_END_OFFSET, SOUND_EXPR_VOLUME_END_LENGTH, args[ARG_vol_end].u_int);
    sound_expr_encode(self, SOUND_EXPR_WAVEFORM_OFFSET, SOUND_EXPR_WAVEFORM_LENGTH, args[ARG_waveform].u_int);
    sound_expr_encode(self, SOUND_EXPR_FX_CHOICE_OFFSET, SOUND_EXPR_FX_CHOICE_LENGTH, args[ARG_fx].u_int);
    sound_expr_encode(self, SOUND_EXPR_SHAPE_OFFSET, SOUND_EXPR_SHAPE_LENGTH, args[ARG_shape].u_int);

    // Return new sound effect object
    return MP_OBJ_FROM_PTR(self);
}

STATIC void microbit_soundeffect_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    microbit_soundeffect_obj_t *self = MP_OBJ_TO_PTR(self_in);
    const soundeffect_attr_t *soundeffect_attr = NULL;
    for (size_t i = 0; i < MP_ARRAY_SIZE(soundeffect_attr_table); ++i) {
        if (soundeffect_attr_table[i].qst == attr) {
            soundeffect_attr = &soundeffect_attr_table[i];
            break;
        }
    }
    if (soundeffect_attr == NULL) {
        // Invalid attribute, set MP_OBJ_SENTINEL to continue lookup in locals dict.
        dest[1] = MP_OBJ_SENTINEL;
        return;
    }
    if (dest[0] == MP_OBJ_NULL) {
        // Load attribute.
        unsigned int value = sound_expr_decode(self, soundeffect_attr->offset, soundeffect_attr->length);
        dest[0] = MP_OBJ_NEW_SMALL_INT(value);
    } else if (dest[1] != MP_OBJ_NULL) {
        // Store attribute.
        if (self->is_mutable) {
            unsigned int value = mp_obj_get_int(dest[1]);
            sound_expr_encode(self, soundeffect_attr->offset, soundeffect_attr->length, value);
            dest[0] = MP_OBJ_NULL; // Indicate store succeeded.
        }
    }
}

STATIC mp_obj_t microbit_soundeffect_from_string(mp_obj_t str_in) {
    microbit_soundeffect_obj_t *self = m_new_obj(microbit_soundeffect_obj_t);
    self->base.type = &microbit_soundeffect_type;
    self->is_mutable = true;

    // Initialise the sound expression data with the preset values.
    memset(&self->sound_expr[0], '0', SOUND_EXPR_TOTAL_LENGTH);
    size_t len;
    const char *str = mp_obj_str_get_data(str_in, &len);
    if (len > SOUND_EXPR_TOTAL_LENGTH) {
        len = SOUND_EXPR_TOTAL_LENGTH;
    }
    memcpy(&self->sound_expr[0], str, len);

    return MP_OBJ_FROM_PTR(self);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(microbit_soundeffect_from_string_obj, microbit_soundeffect_from_string);
STATIC MP_DEFINE_CONST_STATICMETHOD_OBJ(microbit_soundeffect_from_string_staticmethod_obj, MP_ROM_PTR(&microbit_soundeffect_from_string_obj));

STATIC mp_obj_t microbit_soundeffect_copy(mp_obj_t self_in) {
    microbit_soundeffect_obj_t *self = MP_OBJ_TO_PTR(self_in);
    microbit_soundeffect_obj_t *copy = m_new_obj(microbit_soundeffect_obj_t);
    copy->base.type = self->base.type;
    copy->is_mutable = true;
    memcpy(&copy->sound_expr[0], &self->sound_expr[0], SOUND_EXPR_TOTAL_LENGTH);

    return MP_OBJ_FROM_PTR(copy);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(microbit_soundeffect_copy_obj, microbit_soundeffect_copy);

STATIC const mp_rom_map_elem_t microbit_soundeffect_locals_dict_table[] = {
    // Static methods.
    { MP_ROM_QSTR(MP_QSTR__from_string), MP_ROM_PTR(&microbit_soundeffect_from_string_staticmethod_obj) },

    // Instance methods.
    { MP_ROM_QSTR(MP_QSTR_copy), MP_ROM_PTR(&microbit_soundeffect_copy_obj) },

    // Class constants.
    #define C(NAME) { MP_ROM_QSTR(MP_QSTR_ ## NAME), MP_ROM_INT(SOUND_EFFECT_ ## NAME) }

    C(WAVEFORM_SINE),
    C(WAVEFORM_SAWTOOTH),
    C(WAVEFORM_TRIANGLE),
    C(WAVEFORM_SQUARE),
    C(WAVEFORM_NOISE),

    C(SHAPE_LINEAR),
    C(SHAPE_CURVE),
    C(SHAPE_LOG),

    C(FX_NONE),
    C(FX_TREMOLO),
    C(FX_VIBRATO),
    C(FX_WARBLE),

    #undef C
};
STATIC MP_DEFINE_CONST_DICT(microbit_soundeffect_locals_dict, microbit_soundeffect_locals_dict_table);

const mp_obj_type_t microbit_soundeffect_type = {
    { &mp_type_type },
    .name = MP_QSTR_MicroBitSoundEffect,
    .print = microbit_soundeffect_print,
    .make_new = microbit_soundeffect_make_new,
    .attr = microbit_soundeffect_attr,
    .locals_dict = (mp_obj_dict_t *)&microbit_soundeffect_locals_dict,
};
