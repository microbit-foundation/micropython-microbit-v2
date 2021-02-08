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
#ifndef MICROPY_INCLUDED_MICROBIT_MODMICROBIT_H
#define MICROPY_INCLUDED_MICROBIT_MODMICROBIT_H

#include "py/obj.h"
#include "drv_image.h"
#include "drv_display.h"

// Pin modes
#define MODE_UNUSED 1
#define MODE_READ_DIGITAL 2
#define MODE_WRITE_DIGITAL 3
#define MODE_DISPLAY 4
#define MODE_BUTTON 5
#define MODE_MUSIC 6
#define MODE_AUDIO_PLAY 7
#define MODE_TOUCH 8
#define MODE_I2C 9
#define MODE_SPI 10
#define MODE_WRITE_ANALOG 11

#define microbit_pin_mode_unused        (&microbit_pinmodes[MODE_UNUSED])
#define microbit_pin_mode_write_analog  (&microbit_pinmodes[MODE_WRITE_ANALOG])
#define microbit_pin_mode_read_digital  (&microbit_pinmodes[MODE_READ_DIGITAL])
#define microbit_pin_mode_write_digital (&microbit_pinmodes[MODE_WRITE_DIGITAL])
#define microbit_pin_mode_display       (&microbit_pinmodes[MODE_DISPLAY])
#define microbit_pin_mode_button        (&microbit_pinmodes[MODE_BUTTON])
#define microbit_pin_mode_music         (&microbit_pinmodes[MODE_MUSIC])
#define microbit_pin_mode_audio_play    (&microbit_pinmodes[MODE_AUDIO_PLAY])
#define microbit_pin_mode_touch         (&microbit_pinmodes[MODE_TOUCH])
#define microbit_pin_mode_i2c           (&microbit_pinmodes[MODE_I2C])
#define microbit_pin_mode_spi           (&microbit_pinmodes[MODE_SPI])

#define microbit_pin_default_audio_obj (microbit_p0_obj)

typedef struct _microbit_pin_obj_t {
    mp_obj_base_t base;
    uint8_t number; // The pin number on microbit board
    uint8_t name; // The pin number in the GPIO port.
    uint8_t initial_mode;
} microbit_pin_obj_t;

typedef void(*release_func)(const microbit_pin_obj_t *pin);

typedef struct _pinmode {
    qstr name;
    release_func release; /* Call this function to release pin */
} microbit_pinmode_t;

typedef struct _microbit_sound_obj_t {
    mp_obj_base_t base;
    const char *name;
} microbit_sound_obj_t;

typedef struct _microbit_soundevent_obj_t microbit_soundevent_obj_t;

extern const microbit_pinmode_t microbit_pinmodes[];

extern const mp_obj_type_t microbit_ad_pin_type;
extern const mp_obj_type_t microbit_dig_pin_type;
extern const mp_obj_type_t microbit_touch_pin_type;
extern const mp_obj_type_t microbit_touch_only_pin_type;
extern const mp_obj_type_t microbit_sound_type;
extern const mp_obj_type_t microbit_soundevent_type;

extern const struct _microbit_pin_obj_t microbit_p0_obj;
extern const struct _microbit_pin_obj_t microbit_p1_obj;
extern const struct _microbit_pin_obj_t microbit_p2_obj;
extern const struct _microbit_pin_obj_t microbit_p3_obj;
extern const struct _microbit_pin_obj_t microbit_p4_obj;
extern const struct _microbit_pin_obj_t microbit_p5_obj;
extern const struct _microbit_pin_obj_t microbit_p6_obj;
extern const struct _microbit_pin_obj_t microbit_p7_obj;
extern const struct _microbit_pin_obj_t microbit_p8_obj;
extern const struct _microbit_pin_obj_t microbit_p9_obj;
extern const struct _microbit_pin_obj_t microbit_p10_obj;
extern const struct _microbit_pin_obj_t microbit_p11_obj;
extern const struct _microbit_pin_obj_t microbit_p12_obj;
extern const struct _microbit_pin_obj_t microbit_p13_obj;
extern const struct _microbit_pin_obj_t microbit_p14_obj;
extern const struct _microbit_pin_obj_t microbit_p15_obj;
extern const struct _microbit_pin_obj_t microbit_p16_obj;
extern const struct _microbit_pin_obj_t microbit_p19_obj;
extern const struct _microbit_pin_obj_t microbit_p20_obj;
extern const struct _microbit_pin_obj_t microbit_pin_logo_obj;
extern const struct _microbit_pin_obj_t microbit_pin_speaker_obj;

extern const struct _microbit_i2c_obj_t microbit_i2c_obj;
extern const struct _microbit_uart_obj_t microbit_uart_obj;
extern const struct _microbit_spi_obj_t microbit_spi_obj;

extern const struct _monochrome_5by5_t microbit_const_image_heart_obj;
extern const struct _monochrome_5by5_t microbit_const_image_heart_small_obj;
extern const struct _monochrome_5by5_t microbit_const_image_happy_obj;
extern const struct _monochrome_5by5_t microbit_const_image_smile_obj;
extern const struct _monochrome_5by5_t microbit_const_image_sad_obj;
extern const struct _monochrome_5by5_t microbit_const_image_confused_obj;
extern const struct _monochrome_5by5_t microbit_const_image_angry_obj;
extern const struct _monochrome_5by5_t microbit_const_image_asleep_obj;
extern const struct _monochrome_5by5_t microbit_const_image_surprised_obj;
extern const struct _monochrome_5by5_t microbit_const_image_silly_obj;
extern const struct _monochrome_5by5_t microbit_const_image_fabulous_obj;
extern const struct _monochrome_5by5_t microbit_const_image_meh_obj;
extern const struct _monochrome_5by5_t microbit_const_image_yes_obj;
extern const struct _monochrome_5by5_t microbit_const_image_no_obj;
extern const struct _monochrome_5by5_t microbit_const_image_clock12_obj;
extern const struct _monochrome_5by5_t microbit_const_image_clock1_obj;
extern const struct _monochrome_5by5_t microbit_const_image_clock2_obj;
extern const struct _monochrome_5by5_t microbit_const_image_clock3_obj;
extern const struct _monochrome_5by5_t microbit_const_image_clock4_obj;
extern const struct _monochrome_5by5_t microbit_const_image_clock5_obj;
extern const struct _monochrome_5by5_t microbit_const_image_clock6_obj;
extern const struct _monochrome_5by5_t microbit_const_image_clock7_obj;
extern const struct _monochrome_5by5_t microbit_const_image_clock8_obj;
extern const struct _monochrome_5by5_t microbit_const_image_clock9_obj;
extern const struct _monochrome_5by5_t microbit_const_image_clock10_obj;
extern const struct _monochrome_5by5_t microbit_const_image_clock11_obj;
extern const struct _monochrome_5by5_t microbit_const_image_arrow_n_obj;
extern const struct _monochrome_5by5_t microbit_const_image_arrow_ne_obj;
extern const struct _monochrome_5by5_t microbit_const_image_arrow_e_obj;
extern const struct _monochrome_5by5_t microbit_const_image_arrow_se_obj;
extern const struct _monochrome_5by5_t microbit_const_image_arrow_s_obj;
extern const struct _monochrome_5by5_t microbit_const_image_arrow_sw_obj;
extern const struct _monochrome_5by5_t microbit_const_image_arrow_w_obj;
extern const struct _monochrome_5by5_t microbit_const_image_arrow_nw_obj;
extern const struct _monochrome_5by5_t microbit_const_image_triangle_obj;
extern const struct _monochrome_5by5_t microbit_const_image_triangle_left_obj;
extern const struct _monochrome_5by5_t microbit_const_image_chessboard_obj;
extern const struct _monochrome_5by5_t microbit_const_image_diamond_obj;
extern const struct _monochrome_5by5_t microbit_const_image_diamond_small_obj;
extern const struct _monochrome_5by5_t microbit_const_image_square_obj;
extern const struct _monochrome_5by5_t microbit_const_image_square_small_obj;
extern const struct _monochrome_5by5_t microbit_const_image_rabbit_obj;
extern const struct _monochrome_5by5_t microbit_const_image_cow_obj;
extern const struct _monochrome_5by5_t microbit_const_image_music_crotchet_obj;
extern const struct _monochrome_5by5_t microbit_const_image_music_quaver_obj;
extern const struct _monochrome_5by5_t microbit_const_image_music_quavers_obj;
extern const struct _monochrome_5by5_t microbit_const_image_pitchfork_obj;
extern const struct _monochrome_5by5_t microbit_const_image_xmas_obj;
extern const struct _monochrome_5by5_t microbit_const_image_pacman_obj;
extern const struct _monochrome_5by5_t microbit_const_image_target_obj;
extern const struct _mp_obj_tuple_t microbit_const_image_all_clocks_tuple_obj;
extern const struct _mp_obj_tuple_t microbit_const_image_all_arrows_tuple_obj;
extern const struct _monochrome_5by5_t microbit_const_image_tshirt_obj;
extern const struct _monochrome_5by5_t microbit_const_image_rollerskate_obj;
extern const struct _monochrome_5by5_t microbit_const_image_duck_obj;
extern const struct _monochrome_5by5_t microbit_const_image_house_obj;
extern const struct _monochrome_5by5_t microbit_const_image_tortoise_obj;
extern const struct _monochrome_5by5_t microbit_const_image_butterfly_obj;
extern const struct _monochrome_5by5_t microbit_const_image_stickfigure_obj;
extern const struct _monochrome_5by5_t microbit_const_image_ghost_obj;
extern const struct _monochrome_5by5_t microbit_const_image_sword_obj;
extern const struct _monochrome_5by5_t microbit_const_image_giraffe_obj;
extern const struct _monochrome_5by5_t microbit_const_image_skull_obj;
extern const struct _monochrome_5by5_t microbit_const_image_umbrella_obj;
extern const struct _monochrome_5by5_t microbit_const_image_snake_obj;

extern const microbit_soundevent_obj_t microbit_soundevent_loud_obj;
extern const microbit_soundevent_obj_t microbit_soundevent_quiet_obj;

extern struct _microbit_display_obj_t microbit_display_obj;
extern const struct _microbit_accelerometer_obj_t microbit_accelerometer_obj;
extern const struct _microbit_compass_obj_t microbit_compass_obj;
extern const struct _microbit_speaker_obj_t microbit_speaker_obj;
extern const struct _microbit_microphone_obj_t microbit_microphone_obj;
extern const struct _microbit_button_obj_t microbit_button_a_obj;
extern const struct _microbit_button_obj_t microbit_button_b_obj;

const microbit_pin_obj_t *microbit_obj_get_pin(mp_const_obj_t o);
uint8_t microbit_obj_get_pin_name(mp_obj_t o);

// Release pin for use by other modes. Safe to call in an interrupt.
// If pin is NULL or pin already unused, then this is a no-op
void microbit_obj_pin_free(const microbit_pin_obj_t *pin);

// Test if a pin can be acquired.
bool microbit_obj_pin_can_be_acquired(const microbit_pin_obj_t *pin);

// Acquire pin (causing analog/digital modes to release) for mode.
// If pin is already in specified mode, this is a no-op and returns "false".
// Otherwise if the acquisition succeeds then it returns "true".
// Not safe to call in an interrupt as it may raise if pin can't be acquired.
bool microbit_obj_pin_acquire(const microbit_pin_obj_t *pin, const microbit_pinmode_t *mode);

const microbit_pinmode_t *microbit_pin_get_mode(const microbit_pin_obj_t *pin);
void pinmode_error(const microbit_pin_obj_t *pin);

void microbit_pin_audio_speaker_enable(bool enable);
void microbit_pin_audio_select(mp_const_obj_t select);
void microbit_pin_audio_free(void);

MP_DECLARE_CONST_FUN_OBJ_0(microbit_reset_obj);

#endif // MICROPY_INCLUDED_MICROBIT_MODMICROBIT_H
