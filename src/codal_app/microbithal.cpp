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

extern "C" {

#include "microbithal.h"

static NRF52Pin *const pin_obj[] = {
    &uBit.io.speaker,
    &uBit.io.P14,
    &uBit.io.P0,
    &uBit.io.P1,
    &uBit.io.P2,
    &uBit.io.microphone,
    &uBit.io.uart_rx,
    &uBit.io.unused1,
    &uBit.io.scl,
    &uBit.io.P9,
    &uBit.io.P8,
    &uBit.io.P7,
    &uBit.io.P12,
    &uBit.io.P15,
    &uBit.io.P5,
    &uBit.io.row3,
    &uBit.io.sda,
    &uBit.io.P13,
    &uBit.io.unused2,
    &uBit.io.row5,
    &uBit.io.runmic,
    &uBit.io.row1,
    &uBit.io.row2,
    &uBit.io.P11,
    &uBit.io.row4,
    &uBit.io.irq1,
    &uBit.io.P19,
    &uBit.io.unused3,
    &uBit.io.P4,
    &uBit.io.unused4,
    &uBit.io.P10,
    &uBit.io.P3,

    &uBit.io.P20,
    &uBit.io.unused5,
    &uBit.io.P16,
    &uBit.io.unused6,
    &uBit.io.face,
    &uBit.io.P6,

    &uBit.io.col1,
    &uBit.io.col2,
    &uBit.io.col3,
    &uBit.io.col4,
    &uBit.io.col5,
    &uBit.io.buttonA,
    &uBit.io.buttonB,

    &uBit.io.usbTx,
    &uBit.io.usbRx,
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

void microbit_hal_idle(void) {
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
    pin_obj[pin]->setAnalogValue(value);
}

int microbit_hal_pin_is_touched(int pin) {
    return pin_obj[pin]->isTouched();
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
    uBit.display.image.setPixelValue(x, y, bright * 255 / 9);
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

static NRF52PWM *pwm = NULL;

class AudioSource : public DataSource {
public:
    ManagedBuffer buf;
    virtual ManagedBuffer pull() {
        const uint8_t *src = microbit_hal_audio_get_data_callback();
        uint16_t *dest = (uint16_t *)buf.getBytes();
        uint32_t scale = pwm->getSampleRange();
        for (size_t i = 0; i < (size_t)buf.length() / 2; ++i) {
            dest[i] = ((uint32_t)src[i] * scale) >> 8;
        }
        return buf;
    }
};

static AudioSource data_source;

void microbit_hal_audio_init(uint32_t sample_rate) {
    if (pwm == NULL) {
        pwm = new NRF52PWM(NRF_PWM1, data_source, sample_rate, 0);
        pwm->connectPin(uBit.io.speaker, 0);
        pwm->connectPin(uBit.io.P0, 1);
    } else {
        pwm->setSampleRate(sample_rate);
    }
}

void microbit_hal_audio_signal_data_ready(size_t num_samples) {
    if ((size_t)data_source.buf.length() != num_samples * 2) {
        data_source.buf = ManagedBuffer(num_samples * 2);
    }
    pwm->pullRequest();
}

}
