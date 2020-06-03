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
#ifndef MICROPY_INCLUDED_CODAL_PORT_DRV_DISPLAY_H
#define MICROPY_INCLUDED_CODAL_PORT_DRV_DISPLAY_H

#include "drv_image.h"

#define MICROBIT_DISPLAY_WIDTH (5)
#define MICROBIT_DISPLAY_HEIGHT (5)
#define MICROBIT_DISPLAY_MAX_BRIGHTNESS (9)

// Delay in ms in between moving display one column to the left.
#define DEFAULT_SCROLL_SPEED_MS       150

void microbit_display_init(void);
void microbit_display_stop(void);
void microbit_display_update(void);

void microbit_display_clear(void);
void microbit_display_show(microbit_image_obj_t *image);
void microbit_display_scroll(const char *str);
void microbit_display_animate(mp_obj_t iterable, mp_int_t delay, bool clear, bool wait);

#endif // MICROPY_INCLUDED_CODAL_PORT_DRV_DISPLAY_H
