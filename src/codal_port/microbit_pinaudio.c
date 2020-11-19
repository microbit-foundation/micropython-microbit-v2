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

#include "microbithal.h"
#include "modmicrobit.h"

const mp_obj_tuple_t microbit_pin_default_audio_obj = {
    { &mp_type_tuple },
    2,
    {
        (mp_obj_t)MP_ROM_PTR(&microbit_p0_obj),
        (mp_obj_t)MP_ROM_PTR(&microbit_pin_speaker_obj),
    }
};

// Whether the speaker is enabled or not.  If true, this overrides any selection
// of the speaker to act as though it was not selected.
STATIC bool audio_speaker_enabled = true;

// The currently selected pin and/or speaker output for the audio.
STATIC const microbit_pin_obj_t *audio_routed_pin = NULL;
STATIC bool audio_routed_speaker = false;

STATIC void microbit_pin_audio_get_pins(mp_const_obj_t select, const microbit_pin_obj_t **pin_selected, bool *speaker_selected) {
    // Convert the "select" object into an array of items (which should be pins).
    size_t len;
    mp_obj_t *items;
    if (mp_obj_is_type(select, &mp_type_tuple)) {
        mp_obj_get_array((mp_obj_t)select, &len, &items);
        if (len > 2) {
            mp_raise_ValueError(MP_ERROR_TEXT("maximum of 2 pins allowed"));
        }
    } else {
        len = 1;
        items = (mp_obj_t *)&select;
    }

    // Work out which pins are selected for the audio output.
    *pin_selected = NULL;
    *speaker_selected = false;
    if (len == 1 || len == 2) {
        if (items[0] == &microbit_pin_speaker_obj) {
            *speaker_selected = true;
        } else {
            *pin_selected = microbit_obj_get_pin(items[0]);
        }
        if (len == 2) {
            if (items[1] == &microbit_pin_speaker_obj) {
                if (*speaker_selected) {
                    mp_raise_ValueError(MP_ERROR_TEXT("speaker selected twice"));
                }
                *speaker_selected = true;
            } else {
                if (*pin_selected != NULL) {
                    mp_raise_ValueError(MP_ERROR_TEXT("can only select one non-speaker pin"));
                }
                *pin_selected = microbit_obj_get_pin(items[1]);
            }
        }
    }

    // Apply global override to enable/disable speaker.
    *speaker_selected = *speaker_selected && audio_speaker_enabled;
}

void microbit_pin_audio_speaker_enable(bool enable) {
    audio_speaker_enabled = enable;
}

void microbit_pin_audio_select(mp_const_obj_t select) {
    // Work out which pins are selected.
    const microbit_pin_obj_t *pin_selected;
    bool speaker_selected;
    microbit_pin_audio_get_pins(select, &pin_selected, &speaker_selected);

    // Change the pin if needed.
    if (pin_selected != audio_routed_pin) {
        if (audio_routed_pin != NULL) {
            microbit_obj_pin_free(audio_routed_pin);
        }
        audio_routed_pin = pin_selected;
        if (audio_routed_pin == NULL) {
            microbit_hal_audio_select_pin(-1);
        } else {
            microbit_obj_pin_acquire(audio_routed_pin, microbit_pin_mode_music);
            microbit_hal_audio_select_pin(audio_routed_pin->name);
        }
    }

    // Change the speaker mode if needed.
    if (speaker_selected != audio_routed_speaker) {
        audio_routed_speaker = speaker_selected;
        microbit_hal_audio_select_speaker(audio_routed_speaker);
    }
}

void microbit_pin_audio_free(void) {
    if (audio_routed_pin != NULL) {
        microbit_obj_pin_free(audio_routed_pin);
        audio_routed_pin = NULL;
    }
}
