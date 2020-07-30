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

#define MICROPY_TIMER_EVENT (0x1001)

extern "C" void mp_main(void);
extern "C" void m_printf(...);
extern "C" void microbit_hal_timer_callback(void);
extern "C" void microbit_hal_gesture_callback(int);

MicroBit uBit;

void timer_handler(Event evt) {
    microbit_hal_timer_callback();
}

void gesture_event_handler(Event evt) {
    microbit_hal_gesture_callback(evt.value);
}

int main() {
    uBit.init();

    // As well as configuring a larger RX buffer, this needs to be called so it
    // calls Serial::initialiseRx, to set up interrupts.
    uBit.serial.setRxBufferSize(128);

    uBit.messageBus.listen(MICROPY_TIMER_EVENT, DEVICE_EVT_ANY, timer_handler, MESSAGE_BUS_LISTENER_IMMEDIATE);
    uBit.messageBus.listen(DEVICE_ID_SERIAL, CODAL_SERIAL_EVT_DELIM_MATCH, serial_interrupt_handler, MESSAGE_BUS_LISTENER_IMMEDIATE);
    uBit.messageBus.listen(DEVICE_ID_GESTURE, DEVICE_EVT_ANY, gesture_event_handler, MESSAGE_BUS_LISTENER_IMMEDIATE);

    // 6ms follows the micro:bit v1 value
    system_timer_event_every(6, MICROPY_TIMER_EVENT, 1);

    uBit.display.setDisplayMode(DISPLAY_MODE_GREYSCALE);
    uBit.display.setBrightness(255);

    mp_main();
    return 0;
}

extern "C" void __wrap_atexit(void) {
}
