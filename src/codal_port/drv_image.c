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

#include <string.h>
#include "drv_image.h"
#include "drv_display.h"

const monochrome_5by5_t microbit_blank_image = {
    { &microbit_image_type },
    1, 0, 0, 0,
    { 0, 0, 0 }
};

static uint8_t monochrome_get_pixel(monochrome_5by5_t *self, mp_int_t x, mp_int_t y) {
    unsigned int index = y*5+x;
    if (index == 24)
        return self->pixel44;
    return (self->bits24[index>>3] >> (index&7))&1;
}

greyscale_t *greyscale_new(mp_int_t w, mp_int_t h) {
    greyscale_t *result = m_new_obj_var(greyscale_t, byte_data, uint8_t, (w*h+1)>>1);
    result->base.type = &microbit_image_type;
    result->five = 0;
    result->width = w;
    result->height = h;
    return result;
}

void greyscale_clear(greyscale_t *self) {
    memset(&self->byte_data, 0, (self->width*self->height+1)>>1);
}

void greyscale_fill(greyscale_t *self, mp_int_t val) {
    mp_int_t byte = (val<<4) | val;
    for (int i = 0; i < ((self->width*self->height+1)>>1); i++) {
        self->byte_data[i] = byte;
    }
}

uint8_t greyscale_get_pixel(greyscale_t *self, mp_int_t x, mp_int_t y) {
    unsigned int index = y*self->width+x;
    unsigned int shift = ((index<<2)&4);
    return (self->byte_data[index>>1] >> shift)&15;
}

void greyscale_set_pixel(greyscale_t *self, mp_int_t x, mp_int_t y, mp_int_t val) {
    unsigned int index = y*self->width+x;
    unsigned int shift = ((index<<2)&4);
    uint8_t mask = 240 >> shift;
    self->byte_data[index>>1] = (self->byte_data[index>>1] & mask) | (val << shift);
}

mp_int_t image_width(microbit_image_obj_t *self) {
    if (self->base.five) {
        return 5;
    } else {
        return self->greyscale.width;
    }
}

mp_int_t image_height(microbit_image_obj_t *self) {
    if (self->base.five) {
        return 5;
    } else {
        return self->greyscale.height;
    }
}

uint8_t image_get_pixel(microbit_image_obj_t *self, mp_int_t x, mp_int_t y) {
    if (self->base.five) {
        return monochrome_get_pixel(&self->monochrome_5by5, x, y) * MICROBIT_DISPLAY_MAX_BRIGHTNESS;
    } else {
        return greyscale_get_pixel(&self->greyscale, x, y);
    }
}

greyscale_t *image_copy(microbit_image_obj_t *self) {
    mp_int_t w = image_width(self);
    mp_int_t h = image_height(self);
    greyscale_t *result = greyscale_new(w, h);
    for (mp_int_t y = 0; y < h; y++) {
        for (mp_int_t x = 0; x < w; ++x) {
            greyscale_set_pixel(result, x,y, image_get_pixel(self, x,y));
        }
    }
    return result;
}

greyscale_t *image_invert(microbit_image_obj_t *self) {
    mp_int_t w = image_width(self);
    mp_int_t h = image_height(self);
    greyscale_t *result = greyscale_new(w, h);
    for (mp_int_t y = 0; y < h; y++) {
        for (mp_int_t x = 0; x < w; ++x) {
            greyscale_set_pixel(result,x,y, MICROBIT_DISPLAY_MAX_BRIGHTNESS - image_get_pixel(self,x,y));
        }
    }
    return result;
}

static void clear_rect(greyscale_t *img, mp_int_t x0, mp_int_t y0,mp_int_t x1, mp_int_t y1) {
    for (int i = x0; i < x1; ++i) {
        for (int j = y0; j < y1; ++j) {
            greyscale_set_pixel(img, i, j, 0);
        }
    }
}

void image_blit(microbit_image_obj_t *src, greyscale_t *dest, mp_int_t x, mp_int_t y, mp_int_t w, mp_int_t h, mp_int_t xdest, mp_int_t ydest) {
    if (w < 0) {
        w = 0;
    }
    if (h < 0) {
        h = 0;
    }
    mp_int_t intersect_x0 = MAX(MAX(0, x), -xdest);
    mp_int_t intersect_y0 = MAX(MAX(0, y), -ydest);
    mp_int_t intersect_x1 = MIN(MIN(dest->width+x-xdest, image_width(src)), x+w);
    mp_int_t intersect_y1 = MIN(MIN(dest->height+y-ydest, image_height(src)), y+h);
    mp_int_t xstart, xend, ystart, yend, xdel, ydel;
    mp_int_t clear_x0 = MAX(0, xdest);
    mp_int_t clear_y0 = MAX(0, ydest);
    mp_int_t clear_x1 = MIN(dest->width, xdest+w);
    mp_int_t clear_y1 = MIN(dest->height, ydest+h);
    if (intersect_x0 >= intersect_x1 || intersect_y0 >= intersect_y1) {
        // Nothing to copy
        clear_rect(dest, clear_x0, clear_y0, clear_x1, clear_y1);
        return;
    }
    if (x > xdest) {
        xstart = intersect_x0; xend = intersect_x1; xdel = 1;
    } else {
        xstart = intersect_x1 - 1; xend = intersect_x0 - 1; xdel = -1;
    }
    if (y > ydest) {
        ystart = intersect_y0; yend = intersect_y1; ydel = 1;
    } else {
        ystart = intersect_y1 - 1; yend = intersect_y0 - 1; ydel = -1;
    }
    for (int i = xstart; i != xend; i += xdel) {
        for (int j = ystart; j != yend; j += ydel) {
            int val = image_get_pixel(src, i, j);
            greyscale_set_pixel(dest, i+xdest-x, j+ydest-y, val);
        }
    }
    // Adjust intersection rectange to dest
    intersect_x0 += xdest - x;
    intersect_y0 += ydest - y;
    intersect_x1 += xdest - x;
    intersect_y1 += ydest - y;
    // Clear four rectangles in the cleared area surrounding the copied area.
    clear_rect(dest, clear_x0, clear_y0, intersect_x0, intersect_y1);
    clear_rect(dest, clear_x0, intersect_y1, intersect_x1, clear_y1);
    clear_rect(dest, intersect_x1, intersect_y0, clear_x1, clear_y1);
    clear_rect(dest, intersect_x0, clear_y0, clear_x1, intersect_y0);
}
