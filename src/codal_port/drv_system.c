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
#include "drv_system.h"
#include "drv_display.h"
#include "modmusic.h"

extern volatile bool accelerometer_up_to_date;

void microbit_system_init(void) {
    accelerometer_up_to_date = false;
}

// Called every 6ms on a hardware interrupt.
// TODO: should only enable this when system is ready
// TODO: perhaps only schedule the callback when we need it
void microbit_hal_timer_callback(void) {
    // Invalidate accelerometer data for gestures so sample is taken on next gesture call.
    accelerometer_up_to_date = false;

    microbit_display_update();
    microbit_music_tick();
}

void microbit_hal_serial_interrupt_callback(void) {
    mp_sched_keyboard_interrupt();
}
