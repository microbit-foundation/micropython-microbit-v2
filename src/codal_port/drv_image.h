/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Mark Shannon
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
#ifndef MICROPY_INCLUDED_CODAL_PORT_DRV_IMAGE_H
#define MICROPY_INCLUDED_CODAL_PORT_DRV_IMAGE_H

#include "py/runtime.h"

#define SMALL_IMAGE(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p44) \
{ \
    { &microbit_image_type }, \
    1, 0, 0, (p44), \
    { \
        (p0)|((p1)<<1)|((p2)<<2)|((p3)<<3)|((p4)<<4)|((p5)<<5)|((p6)<<6)|((p7)<<7), \
        (p8)|((p9)<<1)|((p10)<<2)|((p11)<<3)|((p12)<<4)|((p13)<<5)|((p14)<<6)|((p15)<<7), \
        (p16)|((p17)<<1)|((p18)<<2)|((p19)<<3)|((p20)<<4)|((p21)<<5)|((p22)<<6)|((p23)<<7) \
    } \
}

#define BLANK_IMAGE (microbit_image_obj_t *)(&microbit_blank_image)
#define HEART_IMAGE (microbit_image_obj_t *)(&microbit_const_image_heart_obj)

// We reserve a couple of bits so we won't need to modify the
// layout if we need to add more functionality or subtypes.
#define TYPE_AND_FLAGS \
    mp_obj_base_t base; \
    uint8_t five:1; \
    uint8_t reserved1:1; \
    uint8_t reserved2:1

typedef struct _image_base_t {
    TYPE_AND_FLAGS;
} image_base_t;

typedef struct _monochrome_5by5_t {
    TYPE_AND_FLAGS;
    uint8_t pixel44: 1;
    uint8_t bits24[3];
} monochrome_5by5_t;

typedef struct _greyscale_t {
    TYPE_AND_FLAGS;
    uint8_t height;
    uint8_t width;
    uint8_t byte_data[];
} greyscale_t;

typedef union _microbit_image_obj_t {
    image_base_t base;
    monochrome_5by5_t monochrome_5by5;
    greyscale_t greyscale;
} microbit_image_obj_t;

extern const mp_obj_type_t microbit_const_image_type;
extern const mp_obj_type_t microbit_image_type;

extern const monochrome_5by5_t microbit_blank_image;
extern const monochrome_5by5_t microbit_const_image_heart_obj;

greyscale_t *greyscale_new(mp_int_t w, mp_int_t h);
void greyscale_clear(greyscale_t *self);
void greyscale_fill(greyscale_t *self, mp_int_t val);
uint8_t greyscale_get_pixel(greyscale_t *self, mp_int_t x, mp_int_t y);
void greyscale_set_pixel(greyscale_t *self, mp_int_t x, mp_int_t y, mp_int_t val);

mp_int_t image_width(microbit_image_obj_t *self);
mp_int_t image_height(microbit_image_obj_t *self);
uint8_t image_get_pixel(microbit_image_obj_t *self, mp_int_t x, mp_int_t y);
greyscale_t *image_copy(microbit_image_obj_t *self);
greyscale_t *image_invert(microbit_image_obj_t *self);
void image_blit(microbit_image_obj_t *src, greyscale_t *dest, mp_int_t x, mp_int_t y, mp_int_t w, mp_int_t h, mp_int_t xdest, mp_int_t ydest);

// Return a facade object that presents the string as a sequence of images
mp_obj_t microbit_string_facade(mp_obj_t string);

microbit_image_obj_t *microbit_image_for_char(char c);
microbit_image_obj_t *microbit_image_dim(microbit_image_obj_t *lhs, mp_float_t fval);

// ref argument exists so that we can pull a string out of an object and not have it GC'ed while oterating over it
mp_obj_t scrolling_string_image_iterable(const char* str, mp_uint_t len, mp_obj_t ref, bool monospace, bool repeat);

#endif // MICROPY_INCLUDED_CODAL_PORT_DRV_IMAGE_H
