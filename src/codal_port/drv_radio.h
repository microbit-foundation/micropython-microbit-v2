/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016-2020 Damien P. George
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
#ifndef MICROPY_INCLUDED_CODAL_PORT_DRV_RADIO_H
#define MICROPY_INCLUDED_CODAL_PORT_DRV_RADIO_H

// Packets are stored in the queue as a sequence of bytes of the form:
//  len  - byte
//  data - "len" bytes
//  RSSI - byte
//  time - 4 bytes, little endian, microsecond timestamp
// Both "len" and "data" are written by the hardware, the others are computed.
#define MICROBIT_RADIO_PACKET_LEN(p)        ((p)[0])
#define MICROBIT_RADIO_PACKET_PAYLOAD(p)    (&(p)[1])
#define MICROBIT_RADIO_PACKET_RSSI(p, len)  (-(p)[1 + len])
/*
#define MICROBIT_RADIO_PACKET_TIMESTAMP_US(p, len) 
        uint32_t timestamp_us = buf[1 + len + 1]
            | buf[1 + len + 2] << 8
            | buf[1 + len + 3] << 16
            | buf[1 + len + 4] << 24;
            */

#define MICROBIT_RADIO_DEFAULT_MAX_PAYLOAD  (32)
#define MICROBIT_RADIO_DEFAULT_QUEUE_LEN    (3)
#define MICROBIT_RADIO_DEFAULT_CHANNEL      (7)
#define MICROBIT_RADIO_DEFAULT_POWER_DBM    (0)
#define MICROBIT_RADIO_DEFAULT_BASE0        (0x75626974) // "uBit"
#define MICROBIT_RADIO_DEFAULT_PREFIX0      (0)
#define MICROBIT_RADIO_DEFAULT_DATA_RATE    (RADIO_MODE_MODE_Nrf_1Mbit)

#define MICROBIT_RADIO_MAX_CHANNEL          (83) // maximum allowed frequency is 2483.5 MHz

typedef struct _microbit_radio_config_t {
    uint8_t max_payload;    // 1-251 inclusive
    uint8_t queue_len;      // 1-254 inclusive
    uint8_t channel;        // 0-100 inclusive
    int8_t power_dbm;       // one of: -30, -20, -16, -12, -8, -4, 0, 4, 8
    uint32_t base0;         // for BASE0 register
    uint8_t prefix0;        // for PREFIX0 register (lower 8 bits only)
    uint8_t data_rate;      // one of: RADIO_MODE_MODE_Nrf_{250Kbit,1Mbit,2Mbit}
} microbit_radio_config_t;

void microbit_radio_enable(microbit_radio_config_t *config);
void microbit_radio_disable(void);
void microbit_radio_update_config(microbit_radio_config_t *config);
void microbit_radio_send(const void *buf, size_t len, const void *buf2, size_t len2);
const uint8_t *microbit_radio_peek(void);
void microbit_radio_pop(void);

#endif // MICROPY_INCLUDED_CODAL_PORT_DRV_RADIO_H
