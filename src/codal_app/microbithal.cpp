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

#include "main.h"
#include "MicroBitDevice.h"

extern "C" {

#include "microbithal.h"

static NRF52Pin *const pin_obj[] = {
    &uBit.io.speaker,
    &uBit.io.P14,
    &uBit.io.P0,
    &uBit.io.P1,
    &uBit.io.P2,
    &uBit.io.microphone,
    &uBit.io.uart_rx,
    &uBit.io.unused1,
    &uBit.io.scl,
    &uBit.io.P9,
    &uBit.io.P8,
    &uBit.io.P7,
    &uBit.io.P12,
    &uBit.io.P15,
    &uBit.io.P5,
    &uBit.io.row3,
    &uBit.io.sda,
    &uBit.io.P13,
    &uBit.io.unused2,
    &uBit.io.row5,
    &uBit.io.runmic,
    &uBit.io.row1,
    &uBit.io.row2,
    &uBit.io.P11,
    &uBit.io.row4,
    &uBit.io.irq1,
    &uBit.io.P19,
    &uBit.io.unused3,
    &uBit.io.P4,
    &uBit.io.unused4,
    &uBit.io.P10,
    &uBit.io.P3,

    &uBit.io.P20,
    &uBit.io.unused5,
    &uBit.io.P16,
    &uBit.io.unused6,
    &uBit.io.face,
    &uBit.io.P6,
};

static Button *const button_obj[] = {
    &uBit.buttonA,
    &uBit.buttonB,
};

static uint16_t button_state[2];

void microbit_hal_idle(void) {
    __WFI();
}

void microbit_hal_reset(void) {
    microbit_reset();
}

void microbit_hal_panic(int code) {
    microbit_panic(code);
}

int microbit_hal_temperature(void) {
    return uBit.thermometer.getTemperature();
}

int microbit_hal_pin_read(int pin) {
    return pin_obj[pin]->getDigitalValue();
}

void microbit_hal_pin_write(int pin, int value) {
    pin_obj[pin]->setDigitalValue(value);
}

int microbit_hal_pin_is_touched(int pin) {
    return pin_obj[pin]->isTouched();
}

int microbit_hal_button_state(int button, int *was_pressed, int *num_presses) {
    Button *b = button_obj[button];
    if (was_pressed != NULL || num_presses != NULL) {
        uint16_t state = button_state[button];
        int p = b->wasPressed();
        if (p) {
            // Update state based on number of presses since last call.
            // Low bit is "was pressed at least once", upper bits are "number of presses".
            state = (state + (p << 1)) | 1;
        }
        if (was_pressed != NULL) {
            *was_pressed = state & 1;
            state &= ~1;
        }
        if (num_presses != NULL) {
            *num_presses = state >> 1;
            state &= 1;
        }
        button_state[button] = state;
    }
    return b->isPressed();
}

void microbit_hal_display_enable(int value) {
    if (value) {
        uBit.display.enable();
    } else {
        uBit.display.disable();
    }
}

void microbit_hal_display_clear(void) {
    uBit.display.clear();
}

int microbit_hal_display_get_pixel(int x, int y) {
    return uBit.display.image.getPixelValue(x, y);
}

void microbit_hal_display_set_pixel(int x, int y, int bright) {
    uBit.display.image.setPixelValue(x, y, bright * 255 / 9);
}

int microbit_hal_display_read_light_level(void) {
    return uBit.display.readLightLevel();
}

void microbit_hal_accelerometer_get_sample(int axis[3]) {
    Sample3D sample = uBit.accelerometer.getSample();
    axis[0] = sample.x;
    axis[1] = sample.y;
    axis[2] = sample.z;
}

const uint8_t *microbit_hal_get_font_data(char c) {
    return BitmapFont::getSystemFont().get(c);
}

}
