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
#ifndef MICROPY_INCLUDED_CODAL_APP_MICROBITHAL_H
#define MICROPY_INCLUDED_CODAL_APP_MICROBITHAL_H

#define MICROBIT_HAL_PIN_P0     (2)
#define MICROBIT_HAL_PIN_P1     (3)
#define MICROBIT_HAL_PIN_P2     (4)
#define MICROBIT_HAL_PIN_P3     (31)
#define MICROBIT_HAL_PIN_P4     (28)
#define MICROBIT_HAL_PIN_P5     (14)
#define MICROBIT_HAL_PIN_P6     (37)
#define MICROBIT_HAL_PIN_P7     (11)
#define MICROBIT_HAL_PIN_P8     (10)
#define MICROBIT_HAL_PIN_P9     (9)
#define MICROBIT_HAL_PIN_P10    (30)
#define MICROBIT_HAL_PIN_P11    (23)
#define MICROBIT_HAL_PIN_P12    (12)
#define MICROBIT_HAL_PIN_P13    (17)
#define MICROBIT_HAL_PIN_P14    (1)
#define MICROBIT_HAL_PIN_P15    (13)
#define MICROBIT_HAL_PIN_P16    (34)
#define MICROBIT_HAL_PIN_P19    (26)
#define MICROBIT_HAL_PIN_P20    (32)
#define MICROBIT_HAL_PIN_USB_TX (45)
#define MICROBIT_HAL_PIN_USB_RX (46)

// These match the micro:bit v1 constants.
#define MICROBIT_HAL_PIN_PULL_UP (0)
#define MICROBIT_HAL_PIN_PULL_DOWN (1)
#define MICROBIT_HAL_PIN_PULL_NONE (2)

#define MICROBIT_HAL_ACCELEROMETER_EVT_NONE         (0)
#define MICROBIT_HAL_ACCELEROMETER_EVT_TILT_UP      (1)
#define MICROBIT_HAL_ACCELEROMETER_EVT_TILT_DOWN    (2)
#define MICROBIT_HAL_ACCELEROMETER_EVT_TILT_LEFT    (3)
#define MICROBIT_HAL_ACCELEROMETER_EVT_TILT_RIGHT   (4)
#define MICROBIT_HAL_ACCELEROMETER_EVT_FACE_UP      (5)
#define MICROBIT_HAL_ACCELEROMETER_EVT_FACE_DOWN    (6)
#define MICROBIT_HAL_ACCELEROMETER_EVT_FREEFALL     (7)
#define MICROBIT_HAL_ACCELEROMETER_EVT_3G           (8)
#define MICROBIT_HAL_ACCELEROMETER_EVT_6G           (9)
#define MICROBIT_HAL_ACCELEROMETER_EVT_8G           (10)
#define MICROBIT_HAL_ACCELEROMETER_EVT_SHAKE        (11)
#define MICROBIT_HAL_ACCELEROMETER_EVT_2G           (12)

void microbit_hal_idle(void);

void microbit_hal_reset(void);
void microbit_hal_panic(int);
int microbit_hal_temperature(void);

void microbit_hal_pin_set_pull(int pin, int pull);
int microbit_hal_pin_get_pull(int pin);
int microbit_hal_pin_set_analog_period_us(int pin, int period);
int microbit_hal_pin_get_analog_period_us(int pin);
int microbit_hal_pin_read(int pin);
void microbit_hal_pin_write(int pin, int value);
int microbit_hal_pin_read_analog_u10(int pin);
void microbit_hal_pin_write_analog_u10(int pin, int value);
int microbit_hal_pin_is_touched(int pin);

int microbit_hal_i2c_init(int scl, int sda, int freq);
int microbit_hal_i2c_readfrom(uint8_t addr, uint8_t *buf, size_t len, int stop);
int microbit_hal_i2c_writeto(uint8_t addr, const uint8_t *buf, size_t len, int stop);

int microbit_hal_uart_init(int tx, int rx, int baudrate, int bits, int parity, int stop);

int microbit_hal_spi_init(int sclk, int mosi, int miso, int frequency, int bits, int mode);
int microbit_hal_spi_transfer(size_t len, const uint8_t *src, uint8_t *dest);

int microbit_hal_button_state(int button, int *was_pressed, int *num_presses);

void microbit_hal_display_enable(int value);
void microbit_hal_display_clear(void);
int microbit_hal_display_get_pixel(int x, int y);
void microbit_hal_display_set_pixel(int x, int y, int bright);
int microbit_hal_display_read_light_level(void);

void microbit_hal_accelerometer_get_sample(int axis[3]);
int microbit_hal_accelerometer_get_gesture(void);

int microbit_hal_compass_is_calibrated(void);
void microbit_hal_compass_clear_calibration(void);
void microbit_hal_compass_calibrate(void);
void microbit_hal_compass_get_sample(int axis[3]);
int microbit_hal_compass_get_field_strength(void);
int microbit_hal_compass_get_heading(void);

const uint8_t *microbit_hal_get_font_data(char c);

void microbit_hal_audio_init(uint32_t sample_rate);
void microbit_hal_audio_signal_data_ready(size_t num_samples);
const uint8_t *microbit_hal_audio_get_data_callback(void);

#endif // MICROPY_INCLUDED_CODAL_APP_MICROBITHAL_H
