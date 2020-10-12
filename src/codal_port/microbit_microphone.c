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

#include "py/runtime.h"
#include "py/mphal.h"
#include "modmicrobit.h"

#define EVENT_HISTORY_SIZE (8)

#define SOUND_EVENT_NONE (0)
#define SOUND_EVENT_LOUD (1)
#define SOUND_EVENT_QUIET (2)

typedef struct _microbit_microphone_obj_t {
    mp_obj_base_t base;
} microbit_microphone_obj_t;

static const qstr sound_event_name_map[] = {
    [SOUND_EVENT_NONE] = MP_QSTR_,
    [SOUND_EVENT_LOUD] = MP_QSTR_loud,
    [SOUND_EVENT_QUIET] = MP_QSTR_quiet,
};

static uint8_t sound_event_current = 0;
static uint8_t sound_event_active_mask = 0;
static uint8_t sound_event_history_index = 0;
static uint8_t sound_event_history_array[EVENT_HISTORY_SIZE];

void microbit_hal_level_detector_callback(int value) {
    // Work out the sound event.
    uint8_t ev = SOUND_EVENT_NONE;
    if (value == MICROBIT_HAL_MICROPHONE_LEVEL_THRESHOLD_LOW) {
        ev = SOUND_EVENT_QUIET;
    } else if (value == MICROBIT_HAL_MICROPHONE_LEVEL_THRESHOLD_HIGH) {
        ev = SOUND_EVENT_LOUD;
    }

    // Set the sound event as active, and add it to the history.
    sound_event_current = ev;
    if (ev != SOUND_EVENT_NONE) {
        sound_event_active_mask |= 1 << ev;
        if (sound_event_history_index < EVENT_HISTORY_SIZE) {
            sound_event_history_array[sound_event_history_index++] = ev;
        }
    }
}

STATIC void microphone_init(void) {
    microbit_hal_microphone_init();
}

STATIC uint8_t sound_event_from_obj(mp_obj_t sound_in) {
    qstr sound = mp_obj_str_get_qstr(sound_in);
    for (uint8_t i = 0; i < MP_ARRAY_SIZE(sound_event_name_map); ++i) {
        if (sound == sound_event_name_map[i]) {
            return i;
        }
    }
    mp_raise_ValueError("invalid sound");
}

STATIC mp_obj_t microbit_microphone_set_threshold(mp_obj_t self_in, mp_obj_t sound_in, mp_obj_t value_in) {
    (void)self_in;
    uint8_t sound = sound_event_from_obj(sound_in);
    int kind;
    if (sound == SOUND_EVENT_QUIET) {
        kind = 0;
    } else if (sound == SOUND_EVENT_LOUD) {
        kind = 1;
    } else {
        mp_raise_ValueError("invalid sound");
    }
    int value = mp_obj_get_int(value_in);
    microphone_init();
    microbit_hal_microphone_set_threshold(kind, value);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(microbit_microphone_set_threshold_obj, microbit_microphone_set_threshold);

STATIC mp_obj_t microbit_microphone_sound_level(mp_obj_t self_in) {
    (void)self_in;
    microphone_init();
    return MP_OBJ_NEW_SMALL_INT(microbit_hal_microphone_get_level());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(microbit_microphone_sound_level_obj, microbit_microphone_sound_level);

STATIC mp_obj_t microbit_microphone_current_sound(mp_obj_t self_in) {
    (void)self_in;
    microphone_init();
    return MP_OBJ_NEW_QSTR(sound_event_name_map[sound_event_current]);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(microbit_microphone_current_sound_obj, microbit_microphone_current_sound);

STATIC mp_obj_t microbit_microphone_is_sound(mp_obj_t self_in, mp_obj_t sound_in) {
    (void)self_in;
    microphone_init();
    uint8_t sound = sound_event_from_obj(sound_in);
    if (sound_event_history_index > 0) {
        return mp_obj_new_bool(sound == sound_event_history_array[sound_event_history_index - 1]);
    }
    return mp_const_false;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(microbit_microphone_is_sound_obj, microbit_microphone_is_sound);

STATIC mp_obj_t microbit_microphone_was_sound(mp_obj_t self_in, mp_obj_t sound_in) {
    (void)self_in;
    microphone_init();
    uint8_t sound = sound_event_from_obj(sound_in);
    mp_obj_t result = mp_obj_new_bool(sound_event_active_mask & (1 << sound));
    sound_event_active_mask &= ~(1 << sound);
    sound_event_history_index = 0;
    return result;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(microbit_microphone_was_sound_obj, microbit_microphone_was_sound);

STATIC mp_obj_t microbit_microphone_get_sounds(mp_obj_t self_in) {
    (void)self_in;
    microphone_init();
    if (sound_event_history_index == 0) {
        return mp_const_empty_tuple;
    }
    mp_obj_tuple_t *o = (mp_obj_tuple_t *)mp_obj_new_tuple(sound_event_history_index, NULL);
    for (size_t i = 0; i < sound_event_history_index; ++i) {
        uint8_t sound = sound_event_history_array[i];
        o->items[i] = MP_OBJ_NEW_QSTR(sound_event_name_map[sound]);
    }
    sound_event_history_index = 0;
    return o;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(microbit_microphone_get_sounds_obj, microbit_microphone_get_sounds);

STATIC const mp_rom_map_elem_t microbit_microphone_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_set_threshold), MP_ROM_PTR(&microbit_microphone_set_threshold_obj) },
    { MP_ROM_QSTR(MP_QSTR_sound_level), MP_ROM_PTR(&microbit_microphone_sound_level_obj) },
    { MP_ROM_QSTR(MP_QSTR_current_sound), MP_ROM_PTR(&microbit_microphone_current_sound_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_sound), MP_ROM_PTR(&microbit_microphone_is_sound_obj) },
    { MP_ROM_QSTR(MP_QSTR_was_sound), MP_ROM_PTR(&microbit_microphone_was_sound_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_sounds), MP_ROM_PTR(&microbit_microphone_get_sounds_obj) },

    { MP_ROM_QSTR(MP_QSTR_LOUD), MP_ROM_QSTR(MP_QSTR_loud) },
    { MP_ROM_QSTR(MP_QSTR_QUIET), MP_ROM_QSTR(MP_QSTR_quiet) },
};
STATIC MP_DEFINE_CONST_DICT(microbit_microphone_locals_dict, microbit_microphone_locals_dict_table);

const mp_obj_type_t microbit_microphone_type = {
    { &mp_type_type },
    .name = MP_QSTR_MicroBitMicrophone,
    .locals_dict = (mp_obj_dict_t *)&microbit_microphone_locals_dict,
};

const microbit_microphone_obj_t microbit_microphone_obj = {
    { &microbit_microphone_type },
};
