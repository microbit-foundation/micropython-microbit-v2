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
#include "MicroBitDevice.h"
#include "neopixel.h"

NRF52Pin *const pin_obj[] = {
    &uBit.io.P0,
    &uBit.io.P1,
    &uBit.io.P2,
    &uBit.io.P3,
    &uBit.io.P4,
    &uBit.io.P5,
    &uBit.io.P6,
    &uBit.io.P7,
    &uBit.io.P8,
    &uBit.io.P9,
    &uBit.io.P10,
    &uBit.io.P11,
    &uBit.io.P12,
    &uBit.io.P13,
    &uBit.io.P14,
    &uBit.io.P15,
    &uBit.io.P16,
    &uBit.io.P19, // external I2C SCL
    &uBit.io.P20, // external I2C SDA
    &uBit.io.face,
    &uBit.io.speaker,
    &uBit.io.runmic,
    &uBit.io.microphone,
    &uBit.io.sda, // internal I2C
    &uBit.io.scl, // internal I2C
    &uBit.io.row1,
    &uBit.io.row2,
    &uBit.io.row3,
    &uBit.io.row4,
    &uBit.io.row5,
    &uBit.io.usbTx,
    &uBit.io.usbRx,
    &uBit.io.irq1,
};

static Button *const button_obj[] = {
    &uBit.buttonA,
    &uBit.buttonB,
};

static const PullMode pin_pull_mode_mapping[] = {
    PullMode::Up,
    PullMode::Down,
    PullMode::None,
};

static uint8_t pin_pull_state[32 + 6];
static uint16_t button_state[2];

extern "C" {

#include "microbithal.h"

void microbit_hal_background_processing(void) {
    // This call takes about 200us.
    Event(DEVICE_ID_SCHEDULER, DEVICE_SCHEDULER_EVT_IDLE);
}

void microbit_hal_idle(void) {
    microbit_hal_background_processing();
    __WFI();
}

void microbit_hal_reset(void) {
    microbit_reset();
}

void microbit_hal_panic(int code) {
    microbit_panic(code);
}

int microbit_hal_temperature(void) {
    return uBit.thermometer.getTemperature();
}

void microbit_hal_pin_set_pull(int pin, int pull) {
    pin_obj[pin]->setPull(pin_pull_mode_mapping[pull]);
    pin_pull_state[pin] = pull;
}

int microbit_hal_pin_get_pull(int pin) {
    return pin_pull_state[pin];
}

int microbit_hal_pin_set_analog_period_us(int pin, int period) {
    // Change the audio virtual-pin period if the pin is the special mixer pin.
    if (pin == MICROBIT_HAL_PIN_MIXER) {
        uBit.audio.virtualOutputPin.setAnalogPeriodUs(period);
        return 0;
    }

    // Calling setAnalogPeriodUs requires the pin to be in analog-out mode.  So
    // test for this mode by first calling getAnalogPeriodUs, and if it fails then
    // attempt to configure the pin in analog-out mode by calling setAnalogValue.
    if ((ErrorCode)pin_obj[pin]->getAnalogPeriodUs() == DEVICE_NOT_SUPPORTED) {
        if (pin_obj[pin]->setAnalogValue(0) != DEVICE_OK) {
            return -1;
        }
    }

    // Set the analog period.
    if (pin_obj[pin]->setAnalogPeriodUs(period) == DEVICE_OK) {
        return 0;
    } else {
        return -1;
    }
}

int microbit_hal_pin_get_analog_period_us(int pin) {
    int period = pin_obj[pin]->getAnalogPeriodUs();
    if (period != DEVICE_NOT_SUPPORTED) {
        return period;
    } else {
        return -1;
    }
}

void microbit_hal_pin_set_touch_mode(int pin, int mode) {
    pin_obj[pin]->isTouched((TouchMode)mode);
}

int microbit_hal_pin_read(int pin) {
    return pin_obj[pin]->getDigitalValue();
}

void microbit_hal_pin_write(int pin, int value) {
    pin_obj[pin]->setDigitalValue(value);
}

int microbit_hal_pin_read_analog_u10(int pin) {
    return pin_obj[pin]->getAnalogValue();
}

void microbit_hal_pin_write_analog_u10(int pin, int value) {
    if (pin == MICROBIT_HAL_PIN_MIXER) {
        uBit.audio.virtualOutputPin.setAnalogValue(value);
        return;
    }
    pin_obj[pin]->setAnalogValue(value);
}

int microbit_hal_pin_is_touched(int pin) {
    if (pin == MICROBIT_HAL_PIN_FACE) {
        // For touch on the face/logo, delegate to the TouchButton instance.
        return uBit.logo.buttonActive();
    }
    return pin_obj[pin]->isTouched();
}

void microbit_hal_pin_write_ws2812(int pin, const uint8_t *buf, size_t len) {
    neopixel_send_buffer(*pin_obj[pin], buf, len);
}

int microbit_hal_i2c_init(int scl, int sda, int freq) {
    // TODO set pins
    int ret = uBit.i2c.setFrequency(freq);
    if (ret != DEVICE_OK) {
        return ret;;
    }
    return 0;
}

int microbit_hal_i2c_readfrom(uint8_t addr, uint8_t *buf, size_t len, int stop) {
    int ret = uBit.i2c.read(addr << 1, (uint8_t *)buf, len, !stop);
    if (ret != DEVICE_OK) {
        return ret;
    }
    return 0;
}

int microbit_hal_i2c_writeto(uint8_t addr, const uint8_t *buf, size_t len, int stop) {
    int ret = uBit.i2c.write(addr << 1, (uint8_t *)buf, len, !stop);
    if (ret != DEVICE_OK) {
        return ret;
    }
    return 0;
}

int microbit_hal_uart_init(int tx, int rx, int baudrate, int bits, int parity, int stop) {
    // TODO set bits, parity stop
    int ret = uBit.serial.redirect(*pin_obj[tx], *pin_obj[rx]);
    if (ret != DEVICE_OK) {
        return ret;
    }
    ret = uBit.serial.setBaud(baudrate);
    if (ret != DEVICE_OK) {
        return ret;
    }
    return 0;
}

static NRF52SPI *spi = NULL;

int microbit_hal_spi_init(int sclk, int mosi, int miso, int frequency, int bits, int mode) {
    if (spi != NULL) {
        delete spi;
    }
    spi = new NRF52SPI(*pin_obj[mosi], *pin_obj[miso], *pin_obj[sclk], NRF_SPIM2);
    int ret = spi->setFrequency(frequency);
    if (ret != DEVICE_OK) {
        return ret;
    }
    ret = spi->setMode(mode, bits);
    if (ret != DEVICE_OK) {
        return ret;
    }
    return 0;
}

int microbit_hal_spi_transfer(size_t len, const uint8_t *src, uint8_t *dest) {
    int ret;
    if (dest == NULL) {
        ret = spi->transfer(src, len, NULL, 0);
    } else {
        ret = spi->transfer(src, len, dest, len);
    }
    return ret;
}

int microbit_hal_button_state(int button, int *was_pressed, int *num_presses) {
    Button *b = button_obj[button];
    if (was_pressed != NULL || num_presses != NULL) {
        uint16_t state = button_state[button];
        int p = b->wasPressed();
        if (p) {
            // Update state based on number of presses since last call.
            // Low bit is "was pressed at least once", upper bits are "number of presses".
            state = (state + (p << 1)) | 1;
        }
        if (was_pressed != NULL) {
            *was_pressed = state & 1;
            state &= ~1;
        }
        if (num_presses != NULL) {
            *num_presses = state >> 1;
            state &= 1;
        }
        button_state[button] = state;
    }
    return b->isPressed();
}

void microbit_hal_display_enable(int value) {
    if (value) {
        uBit.display.enable();
    } else {
        uBit.display.disable();
    }
}

void microbit_hal_display_clear(void) {
    uBit.display.clear();
}

int microbit_hal_display_get_pixel(int x, int y) {
    return uBit.display.image.getPixelValue(x, y);
}

void microbit_hal_display_set_pixel(int x, int y, int bright) {
    // This mapping is designed to give a set of 10 visually distinct levels.
    static uint8_t bright_map[10] = { 0, 1, 2, 4, 8, 16, 32, 64, 128, 255 };
    if (bright < 0) {
        bright = 0;
    } else if (bright > 9) {
        bright = 9;
    }
    uBit.display.image.setPixelValue(x, y, bright_map[bright]);
}

int microbit_hal_display_read_light_level(void) {
    return uBit.display.readLightLevel();
}

void microbit_hal_accelerometer_get_sample(int axis[3]) {
    Sample3D sample = uBit.accelerometer.getSample();
    axis[0] = sample.x;
    axis[1] = sample.y;
    axis[2] = sample.z;
}

int microbit_hal_accelerometer_get_gesture(void) {
    return uBit.accelerometer.getGesture();
}

int microbit_hal_compass_is_calibrated(void) {
    return uBit.compass.isCalibrated();
}

void microbit_hal_compass_clear_calibration(void) {
    uBit.compass.clearCalibration();
}

void microbit_hal_compass_calibrate(void) {
    uBit.compass.calibrate();
}

void microbit_hal_compass_get_sample(int axis[3]) {
    Sample3D sample = uBit.compass.getSample();
    axis[0] = sample.x;
    axis[1] = sample.y;
    axis[2] = sample.z;
}

int microbit_hal_compass_get_field_strength(void) {
    return uBit.compass.getFieldStrength();
}

int microbit_hal_compass_get_heading(void) {
    return uBit.compass.heading();
}

const uint8_t *microbit_hal_get_font_data(char c) {
    return BitmapFont::getSystemFont().get(c);
}

// This is needed by the microbitfs implementation.
uint32_t rng_generate_random_word(void) {
    return uBit.random(65536) << 16 | uBit.random(65536);
}

}
