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
#include "modaudio.h"
#include "modmicrobit.h"

#define EVENT_HISTORY_SIZE (8)

#define SOUND_EVENT_QUIET (0)
#define SOUND_EVENT_LOUD (1)
#define SOUND_EVENT_CLAP (2)

typedef struct _microbit_microphone_obj_t {
    mp_obj_base_t base;
} microbit_microphone_obj_t;

static const mp_const_obj_t sound_event_obj_map[] = {
    [SOUND_EVENT_QUIET] = MP_ROM_PTR(&microbit_soundevent_quiet_obj),
    [SOUND_EVENT_LOUD] = MP_ROM_PTR(&microbit_soundevent_loud_obj),
    [SOUND_EVENT_CLAP] = MP_ROM_PTR(&microbit_soundevent_clap_obj),
};

static uint8_t sound_event_current = SOUND_EVENT_QUIET;
static uint8_t sound_event_active_mask = 0;
static uint8_t sound_event_history_index = 0;
static uint8_t sound_event_history_array[EVENT_HISTORY_SIZE];

void microbit_hal_level_detector_callback(int value) {
    // Work out the sound event.
    uint8_t ev;
    if (value == MICROBIT_HAL_MICROPHONE_EVT_THRESHOLD_LOW) {
        ev = SOUND_EVENT_QUIET;
    } else if (value == MICROBIT_HAL_MICROPHONE_EVT_THRESHOLD_HIGH) {
        ev = SOUND_EVENT_LOUD;
    } else if (value == MICROBIT_HAL_MICROPHONE_EVT_CLAP) {
        ev = SOUND_EVENT_CLAP;
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

static void microphone_init(void) {
    microbit_hal_microphone_init();
}

static uint8_t sound_event_from_obj(mp_obj_t sound) {
    for (uint8_t i = 0; i < MP_ARRAY_SIZE(sound_event_obj_map); ++i) {
        if (sound == sound_event_obj_map[i]) {
            return i;
        }
    }
    mp_raise_ValueError(MP_ERROR_TEXT("invalid sound"));
}

static mp_obj_t microbit_microphone_set_sensitivity(mp_obj_t self_in, mp_obj_t value_in) {
    (void)self_in;
    microbit_hal_microphone_set_sensitivity(mp_obj_get_float(value_in));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(microbit_microphone_set_sensitivity_obj, microbit_microphone_set_sensitivity);

static mp_obj_t microbit_microphone_set_threshold(mp_obj_t self_in, mp_obj_t sound_in, mp_obj_t value_in) {
    (void)self_in;
    uint8_t sound = sound_event_from_obj(sound_in);
    int kind;
    if (sound == SOUND_EVENT_QUIET) {
        kind = MICROBIT_HAL_MICROPHONE_SET_THRESHOLD_LOW;
    } else if (sound == SOUND_EVENT_LOUD) {
        kind = MICROBIT_HAL_MICROPHONE_SET_THRESHOLD_HIGH;
    } else {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid sound"));
    }
    int value = mp_obj_get_int(value_in);
    microphone_init();
    microbit_hal_microphone_set_threshold(kind, value);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(microbit_microphone_set_threshold_obj, microbit_microphone_set_threshold);

static mp_obj_t microbit_microphone_sound_level(mp_obj_t self_in) {
    (void)self_in;
    microphone_init();
    return MP_OBJ_NEW_SMALL_INT(microbit_hal_microphone_get_level());
}
static MP_DEFINE_CONST_FUN_OBJ_1(microbit_microphone_sound_level_obj, microbit_microphone_sound_level);

static mp_obj_t microbit_microphone_sound_level_db(mp_obj_t self_in) {
    (void)self_in;
    microphone_init();
    return mp_obj_new_float_from_f(microbit_hal_microphone_get_level_db());
}
static MP_DEFINE_CONST_FUN_OBJ_1(microbit_microphone_sound_level_db_obj, microbit_microphone_sound_level_db);

static mp_obj_t microbit_microphone_current_event(mp_obj_t self_in) {
    (void)self_in;
    microphone_init();
    return (mp_obj_t)sound_event_obj_map[sound_event_current];
}
static MP_DEFINE_CONST_FUN_OBJ_1(microbit_microphone_current_event_obj, microbit_microphone_current_event);

static mp_obj_t microbit_microphone_is_event(mp_obj_t self_in, mp_obj_t sound_in) {
    (void)self_in;
    microphone_init();
    uint8_t sound = sound_event_from_obj(sound_in);
    return mp_obj_new_bool(sound == sound_event_current);
}
static MP_DEFINE_CONST_FUN_OBJ_2(microbit_microphone_is_event_obj, microbit_microphone_is_event);

static mp_obj_t microbit_microphone_was_event(mp_obj_t self_in, mp_obj_t sound_in) {
    (void)self_in;
    microphone_init();
    uint8_t sound = sound_event_from_obj(sound_in);
    mp_obj_t result = mp_obj_new_bool(sound_event_active_mask & (1 << sound));
    sound_event_active_mask &= ~(1 << sound);
    sound_event_history_index = 0;
    return result;
}
static MP_DEFINE_CONST_FUN_OBJ_2(microbit_microphone_was_event_obj, microbit_microphone_was_event);

static mp_obj_t microbit_microphone_get_events(mp_obj_t self_in) {
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
static MP_DEFINE_CONST_FUN_OBJ_1(microbit_microphone_get_events_obj, microbit_microphone_get_events);

static void microbit_microphone_record_helper(microbit_audio_frame_obj_t *audio_frame, int rate, bool wait) {
    // Set the rate of the AudioFrame, if specified.
    if (rate > 0) {
        audio_frame->rate = rate;
    }

    // Start the recording.
    microbit_hal_microphone_start_recording(audio_frame->data, audio_frame->alloc_size, &audio_frame->used_size, audio_frame->rate);

    if (wait) {
        // Wait for the recording to finish.
        while (microbit_hal_microphone_is_recording()) {
            mp_handle_pending(true);
            microbit_hal_idle();
        }
    }
}

static mp_obj_t microbit_microphone_record(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_duration, ARG_rate, };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_duration, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_rate, MP_ARG_INT, {.u_int = 7812} },
    };

    // Parse the args.
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // Create the AudioFrame to record into.
    size_t size = args[ARG_duration].u_int * args[ARG_rate].u_int / 1000;
    microbit_audio_frame_obj_t *audio_frame = microbit_audio_frame_make_new(size, args[ARG_rate].u_int);

    // Start recording and wait.
    microbit_microphone_record_helper(audio_frame, args[ARG_rate].u_int, true);

    // Return the new AudioFrame.
    return MP_OBJ_FROM_PTR(audio_frame);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(microbit_microphone_record_obj, 1, microbit_microphone_record);

static mp_obj_t microbit_microphone_record_into(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_buffer, ARG_rate, ARG_wait, };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_buffer, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_rate, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_wait, MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = true} },
    };

    // Parse the args.
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // Check that the buffer is an AudioFrame instance.
    if (!mp_obj_is_type(args[ARG_buffer].u_obj, &microbit_audio_frame_type)) {
        mp_raise_TypeError(MP_ERROR_TEXT("expecting an AudioFrame"));
    }
    microbit_audio_frame_obj_t *audio_frame = MP_OBJ_TO_PTR(args[ARG_buffer].u_obj);

    // Start recording and wait if requested.
    microbit_microphone_record_helper(audio_frame, args[ARG_rate].u_int, args[ARG_wait].u_bool);

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(microbit_microphone_record_into_obj, 1, microbit_microphone_record_into);

static mp_obj_t microbit_microphone_is_recording(mp_obj_t self_in) {
    (void)self_in;
    return mp_obj_new_bool(microbit_hal_microphone_is_recording());
}
static MP_DEFINE_CONST_FUN_OBJ_1(microbit_microphone_is_recording_obj, microbit_microphone_is_recording);

static mp_obj_t microbit_microphone_stop_recording(mp_obj_t self_in) {
    (void)self_in;
    microbit_hal_microphone_stop_recording();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(microbit_microphone_stop_recording_obj, microbit_microphone_stop_recording);

#if MICROPY_OBJ_REPR == MICROPY_OBJ_REPR_A || MICROPY_OBJ_REPR == MICROPY_OBJ_REPR_B

// The way float objects are defined depends on the object representation.  Here we only
// support representation A and B which use the following struct for float objects.
// This is defined in py/objfloat.c and not exposed publicly, so must be defined here.

typedef struct _mp_obj_float_t {
    mp_obj_base_t base;
    mp_float_t value;
} mp_obj_float_t;

static const mp_obj_float_t microbit_microphone_sensitivity_low_obj = {{&mp_type_float}, (mp_float_t)0.079};
static const mp_obj_float_t microbit_microphone_sensitivity_medium_obj = {{&mp_type_float}, (mp_float_t)0.2};
static const mp_obj_float_t microbit_microphone_sensitivity_high_obj = {{&mp_type_float}, (mp_float_t)1.0};

#endif

static const mp_rom_map_elem_t microbit_microphone_locals_dict_table[] = {
    // Methods.
    { MP_ROM_QSTR(MP_QSTR_set_sensitivity), MP_ROM_PTR(&microbit_microphone_set_sensitivity_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_threshold), MP_ROM_PTR(&microbit_microphone_set_threshold_obj) },
    { MP_ROM_QSTR(MP_QSTR_sound_level), MP_ROM_PTR(&microbit_microphone_sound_level_obj) },
    { MP_ROM_QSTR(MP_QSTR_sound_level_db), MP_ROM_PTR(&microbit_microphone_sound_level_db_obj) },
    { MP_ROM_QSTR(MP_QSTR_current_event), MP_ROM_PTR(&microbit_microphone_current_event_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_event), MP_ROM_PTR(&microbit_microphone_is_event_obj) },
    { MP_ROM_QSTR(MP_QSTR_was_event), MP_ROM_PTR(&microbit_microphone_was_event_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_events), MP_ROM_PTR(&microbit_microphone_get_events_obj) },
    { MP_ROM_QSTR(MP_QSTR_record), MP_ROM_PTR(&microbit_microphone_record_obj) },
    { MP_ROM_QSTR(MP_QSTR_record_into), MP_ROM_PTR(&microbit_microphone_record_into_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_recording), MP_ROM_PTR(&microbit_microphone_is_recording_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop_recording), MP_ROM_PTR(&microbit_microphone_stop_recording_obj) },

    // Constants.
    { MP_ROM_QSTR(MP_QSTR_SENSITIVITY_LOW), MP_ROM_PTR(&microbit_microphone_sensitivity_low_obj) },
    { MP_ROM_QSTR(MP_QSTR_SENSITIVITY_MEDIUM), MP_ROM_PTR(&microbit_microphone_sensitivity_medium_obj) },
    { MP_ROM_QSTR(MP_QSTR_SENSITIVITY_HIGH), MP_ROM_PTR(&microbit_microphone_sensitivity_high_obj) },
};
static MP_DEFINE_CONST_DICT(microbit_microphone_locals_dict, microbit_microphone_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    microbit_microphone_type,
    MP_QSTR_MicroBitMicrophone,
    MP_TYPE_FLAG_NONE,
    locals_dict, &microbit_microphone_locals_dict
    );

const microbit_microphone_obj_t microbit_microphone_obj = {
    { &microbit_microphone_type },
};
