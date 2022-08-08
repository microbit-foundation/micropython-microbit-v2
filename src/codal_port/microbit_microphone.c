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

#define SOUND_EVENT_QUIET (0)
#define SOUND_EVENT_LOUD (1)

typedef struct _microbit_microphone_obj_t {
    mp_obj_base_t base;
} microbit_microphone_obj_t;

static const mp_const_obj_t sound_event_obj_map[] = {
    [SOUND_EVENT_QUIET] = MP_ROM_PTR(&microbit_soundevent_quiet_obj),
    [SOUND_EVENT_LOUD] = MP_ROM_PTR(&microbit_soundevent_loud_obj),
};

static uint8_t sound_event_current = SOUND_EVENT_QUIET;
static uint8_t sound_event_active_mask = 0;
static uint8_t sound_event_history_index = 0;
static uint8_t sound_event_history_array[EVENT_HISTORY_SIZE];

void microbit_hal_level_detector_callback(int value) {
    // Work out the sound event.
    uint8_t ev;
    if (value == MICROBIT_HAL_MICROPHONE_LEVEL_THRESHOLD_LOW) {
        ev = SOUND_EVENT_QUIET;
    } else if (value == MICROBIT_HAL_MICROPHONE_LEVEL_THRESHOLD_HIGH) {
        ev = SOUND_EVENT_LOUD;
    } else {
        // Ignore unknown events.
        return;
    }

    // Set the sound event as active, and add it to the history.
    sound_event_current = ev;
    sound_event_active_mask |= 1 << ev;
    if (sound_event_history_index < EVENT_HISTORY_SIZE) {
        sound_event_history_array[sound_event_history_index++] = ev;
    }
}

STATIC void microphone_init(void) {
    microbit_hal_microphone_init();
}

STATIC uint8_t sound_event_from_obj(mp_obj_t sound) {
    for (uint8_t i = 0; i < MP_ARRAY_SIZE(sound_event_obj_map); ++i) {
        if (sound == sound_event_obj_map[i]) {
            return i;
        }
    }
    mp_raise_ValueError(MP_ERROR_TEXT("invalid sound"));
}

STATIC mp_obj_t microbit_microphone_set_threshold(mp_obj_t self_in, mp_obj_t sound_in, mp_obj_t value_in) {
    (void)self_in;
    uint8_t sound = sound_event_from_obj(sound_in);
    int kind;
    if (sound == SOUND_EVENT_QUIET) {
        kind = MICROBIT_HAL_MICROPHONE_LEVEL_THRESHOLD_LOW;
    } else if (sound == SOUND_EVENT_LOUD) {
        kind = MICROBIT_HAL_MICROPHONE_LEVEL_THRESHOLD_HIGH;
    } else {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid sound"));
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

STATIC mp_obj_t microbit_microphone_current_event(mp_obj_t self_in) {
    (void)self_in;
    microphone_init();
    return (mp_obj_t)sound_event_obj_map[sound_event_current];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(microbit_microphone_current_event_obj, microbit_microphone_current_event);

STATIC mp_obj_t microbit_microphone_is_event(mp_obj_t self_in, mp_obj_t sound_in) {
    (void)self_in;
    microphone_init();
    uint8_t sound = sound_event_from_obj(sound_in);
    return mp_obj_new_bool(sound == sound_event_current);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(microbit_microphone_is_event_obj, microbit_microphone_is_event);

STATIC mp_obj_t microbit_microphone_was_event(mp_obj_t self_in, mp_obj_t sound_in) {
    (void)self_in;
    microphone_init();
    uint8_t sound = sound_event_from_obj(sound_in);
    mp_obj_t result = mp_obj_new_bool(sound_event_active_mask & (1 << sound));
    sound_event_active_mask &= ~(1 << sound);
    sound_event_history_index = 0;
    return result;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(microbit_microphone_was_event_obj, microbit_microphone_was_event);

STATIC mp_obj_t microbit_microphone_get_events(mp_obj_t self_in) {
    (void)self_in;
    microphone_init();
    if (sound_event_history_index == 0) {
        return mp_const_empty_tuple;
    }
    mp_obj_tuple_t *o = (mp_obj_tuple_t *)mp_obj_new_tuple(sound_event_history_index, NULL);
    for (size_t i = 0; i < sound_event_history_index; ++i) {
        uint8_t sound = sound_event_history_array[i];
        o->items[i] = (mp_obj_t)sound_event_obj_map[sound];
    }
    sound_event_history_index = 0;
    return o;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(microbit_microphone_get_events_obj, microbit_microphone_get_events);

STATIC const mp_rom_map_elem_t microbit_microphone_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_set_threshold), MP_ROM_PTR(&microbit_microphone_set_threshold_obj) },
    { MP_ROM_QSTR(MP_QSTR_sound_level), MP_ROM_PTR(&microbit_microphone_sound_level_obj) },
    { MP_ROM_QSTR(MP_QSTR_current_event), MP_ROM_PTR(&microbit_microphone_current_event_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_event), MP_ROM_PTR(&microbit_microphone_is_event_obj) },
    { MP_ROM_QSTR(MP_QSTR_was_event), MP_ROM_PTR(&microbit_microphone_was_event_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_events), MP_ROM_PTR(&microbit_microphone_get_events_obj) },
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
