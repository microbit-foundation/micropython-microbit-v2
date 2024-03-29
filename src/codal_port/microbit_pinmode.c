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
#include "modaudio.h"
#include "modmicrobit.h"
#include "modmusic.h"

uint8_t microbit_pinmode_indices[32] = { 0 };

const microbit_pinmode_t *microbit_pin_get_mode(const microbit_pin_obj_t *pin) {
    uint8_t pinmode = microbit_pinmode_indices[pin->number];
    if (pinmode == 0) {
        pinmode = pin->initial_mode;
    }
    return &microbit_pinmodes[pinmode];
}

void microbit_pin_set_mode(const microbit_pin_obj_t *pin, const microbit_pinmode_t *mode) {
    uint32_t index = mode - &microbit_pinmodes[0];
    microbit_pinmode_indices[pin->number] = index;
    return;
}

void microbit_obj_pin_free(const microbit_pin_obj_t *pin) {
    if (pin != NULL) {
        microbit_pin_set_mode(pin, microbit_pin_mode_unused);
    }
}

bool microbit_obj_pin_can_be_acquired(const microbit_pin_obj_t *pin) {
    const microbit_pinmode_t *current_mode = microbit_pin_get_mode(pin);
    return current_mode->release != pinmode_error;
}

bool microbit_obj_pin_acquire(const microbit_pin_obj_t *pin, const microbit_pinmode_t *new_mode) {
    const microbit_pinmode_t *current_mode = microbit_pin_get_mode(pin);

    // The button mode is effectively a digital-in mode, so allow read_digital to work on a button
    if (current_mode == microbit_pin_mode_button && new_mode == microbit_pin_mode_read_digital) {
        return false;
    }

    if (current_mode != new_mode) {
        current_mode->release(pin);
        microbit_pin_set_mode(pin, new_mode);
        return true;
    } else {
        return false;
    }
}

void microbit_obj_pin_acquire_and_free(const microbit_pin_obj_t **old_pin, const microbit_pin_obj_t *new_pin, const microbit_pinmode_t *new_mode) {
    microbit_obj_pin_acquire(new_pin, new_mode);
    if (*old_pin != new_pin) {
        microbit_obj_pin_free(*old_pin);
        *old_pin = new_pin;
    }
}

static void noop(const microbit_pin_obj_t *pin) {
    (void)pin;
}

void pinmode_error(const microbit_pin_obj_t *pin) {
    const microbit_pinmode_t *current_mode = microbit_pin_get_mode(pin);
    nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("Pin %d in %q mode"), pin->number, current_mode->name));
}

static void analog_release(const microbit_pin_obj_t *pin) {
    // TODO: pwm_release()
}

static void audio_music_release(const microbit_pin_obj_t *pin) {
    if (microbit_audio_is_playing() || microbit_music_is_playing()) {
        pinmode_error(pin);
    } else {
        microbit_pin_audio_free();
    }
}

const microbit_pinmode_t microbit_pinmodes[] = {
    [MODE_UNUSED]        = { MP_QSTR_unused, noop },
    [MODE_WRITE_ANALOG]  = { MP_QSTR_write_analog, analog_release },
    [MODE_READ_DIGITAL]  = { MP_QSTR_read_digital, noop },
    [MODE_WRITE_DIGITAL] = { MP_QSTR_write_digital, noop },
    [MODE_DISPLAY]       = { MP_QSTR_display, pinmode_error },
    [MODE_BUTTON]        = { MP_QSTR_button, pinmode_error },
    [MODE_MUSIC]         = { MP_QSTR_music, audio_music_release },
    [MODE_AUDIO_PLAY]    = { MP_QSTR_audio, audio_music_release },
    [MODE_TOUCH]         = { MP_QSTR_touch, noop },
    [MODE_I2C]           = { MP_QSTR_i2c, pinmode_error },
    [MODE_SPI]           = { MP_QSTR_spi, pinmode_error }
};
