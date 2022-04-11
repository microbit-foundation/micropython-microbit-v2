/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Damien P. George
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
#ifndef MICROPY_INCLUDED_CODAL_PORT_DRV_SOFTTIMER_H
#define MICROPY_INCLUDED_CODAL_PORT_DRV_SOFTTIMER_H

#include "py/pairheap.h"

#define MICROBIT_SOFT_TIMER_FLAG_PY_CALLBACK (1)
#define MICROBIT_SOFT_TIMER_FLAG_GC_ALLOCATED (2)

#define MICROBIT_SOFT_TIMER_MODE_ONE_SHOT (1)
#define MICROBIT_SOFT_TIMER_MODE_PERIODIC (2)

typedef struct _microbit_soft_timer_entry_t {
    mp_pairheap_t pairheap;
    uint16_t flags;
    uint16_t mode;
    uint32_t expiry_ms;
    uint32_t delta_ms; // for periodic mode
    union {
        void (*c_callback)(struct _microbit_soft_timer_entry_t *);
        mp_obj_t py_callback;
    };
} microbit_soft_timer_entry_t;

extern bool microbit_outer_nlr_will_handle_soft_timer_exceptions;

void microbit_soft_timer_deinit(void);
void microbit_soft_timer_handler(void);
void microbit_soft_timer_insert(microbit_soft_timer_entry_t *entry, uint32_t initial_delta_ms);

#endif // MICROPY_INCLUDED_CODAL_PORT_DRV_SOFTTIMER_H
