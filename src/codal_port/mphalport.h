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

#include "py/obj.h"
#include "microbithal.h"
#include "nrf.h"

// Not implemented and not exposed in utime module.
#define mp_hal_ticks_cpu() (0)

void mp_hal_set_interrupt_char(int c);

static inline uint32_t mp_hal_disable_irq(void) {
    uint32_t state = __get_PRIMASK();
    __disable_irq();
    return state;
}

static inline void mp_hal_enable_irq(uint32_t state) {
    __set_PRIMASK(state);
}

static inline void mp_hal_unique_id(uint32_t id[2]) {
    id[0] = NRF_FICR->DEVICEID[0];
    id[1] = NRF_FICR->DEVICEID[1];
}

static inline uint64_t mp_hal_time_ns(void) {
    // Not currently implemented.
    return 0;
}

// MicroPython low-level C API for pins
#include "modmicrobit.h"
#define mp_hal_pin_obj_t uint8_t
#define mp_hal_get_pin_obj(obj) microbit_obj_get_pin_name(obj)
#define mp_hal_pin_read(pin) microbit_hal_pin_read(pin)
