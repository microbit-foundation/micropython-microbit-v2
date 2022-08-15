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
#include "microbithal.h"
#include "MicroBitDevice.h"

#define SOUND_LEVEL_MAXIMUM (20000)

extern "C" void microbit_hal_level_detector_callback(int);

static void level_detector_event_handler(Event evt) {
    microbit_hal_level_detector_callback(evt.value);
}

extern "C" {

static bool microphone_init_done = false;

void microbit_hal_microphone_init(void) {
    if (!microphone_init_done) {
        microphone_init_done = true;
        uBit.messageBus.listen(DEVICE_ID_SYSTEM_LEVEL_DETECTOR, DEVICE_EVT_ANY, level_detector_event_handler);
    }
}

void microbit_hal_microphone_set_threshold(int kind, int value) {
    value = value * SOUND_LEVEL_MAXIMUM / 255;
    if (kind == 0) {
        uBit.audio.level->setLowThreshold(value);
    } else {
        uBit.audio.level->setHighThreshold(value);
    }
}

int microbit_hal_microphone_get_level(void) {
    int l = uBit.audio.level->getValue();
    l = min(255, l * 255 / SOUND_LEVEL_MAXIMUM);
    return l;
}

}
