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

#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "py/mphal.h"
#include "drv_radio.h"

#define RADIO_PACKET_OVERHEAD (1 + 1 + 4) // 1 byte for len, 1 byte for RSSI, 4 bytes for time

static uint8_t *rx_buf_end = NULL; // pointer to the end of the allocated RX queue
static uint8_t *rx_buf = NULL; // pointer to last packet on the RX queue

void microbit_radio_irq_handler(void) {
    if (NRF_RADIO->EVENTS_READY) {
        NRF_RADIO->EVENTS_READY = 0;
        NRF_RADIO->TASKS_START = 1;
    }

    if (NRF_RADIO->EVENTS_END) {
        NRF_RADIO->EVENTS_END = 0;

        size_t max_len = NRF_RADIO->PCNF1 & 0xff;
        uint8_t *pkt = MP_STATE_PORT(radio_buf);
        size_t len = pkt[0];
        if (len > max_len) {
            len = max_len;
            pkt[0] = len;
        }

        // if the CRC was valid, and there's enough room in the RX queue, then accept the packet
        if (NRF_RADIO->CRCSTATUS == 1 && rx_buf + RADIO_PACKET_OVERHEAD + len <= rx_buf_end) {
            // copy the data to the queue
            memcpy(rx_buf, pkt, 1 + len);

            // store RSSI as last byte in packet (needs to be negated to get actual dBm value)
            rx_buf[1 + len] = NRF_RADIO->RSSISAMPLE;

            // get and store the microsecond timestamp
            uint32_t time = mp_hal_ticks_us();
            rx_buf[1 + len + 1] = time & 0xff;
            rx_buf[1 + len + 2] = (time >> 8) & 0xff;
            rx_buf[1 + len + 3] = (time >> 16) & 0xff;
            rx_buf[1 + len + 4] = (time >> 24) & 0xff;

            // move the RX queue pointer to end of this new packet
            rx_buf += RADIO_PACKET_OVERHEAD + len;
        }

        NRF_RADIO->TASKS_START = 1;
    }
}

void microbit_radio_enable(microbit_radio_config_t *config) {
    microbit_radio_disable();

    // allocate tx and rx buffers
    size_t max_payload = config->max_payload + RADIO_PACKET_OVERHEAD;
    size_t queue_len = config->queue_len + 1; // one extra for tx/rx buffer
    MP_STATE_PORT(radio_buf) = m_new(uint8_t, max_payload * queue_len);
    rx_buf_end = MP_STATE_PORT(radio_buf) + max_payload * queue_len;
    rx_buf = MP_STATE_PORT(radio_buf) + max_payload; // start is tx/rx buffer

    // Enable the High Frequency clock on the processor. This is a pre-requisite for
    // the RADIO module. Without this clock, no communication is possible.
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_HFCLKSTART = 1;
    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0) {
    }

    // power should be one of: -30, -20, -16, -12, -8, -4, 0, 4, 8
    NRF_RADIO->TXPOWER = config->power_dbm;

    // should be between 0 and 100 inclusive (actual physical freq is 2400MHz + this register)
    NRF_RADIO->FREQUENCY = config->channel;

    // configure data rate
    NRF_RADIO->MODE = config->data_rate;

    // The radio supports filtering packets at the hardware level based on an address.
    // We use a 5-byte address comprised of 4 bytes (set by BALEN=4 below) from the BASEx
    // register, plus 1 byte from PREFIXm.APn.
    // The (x,m,n) values are selected by the logical address.  We use logical address 0
    // which means using BASE0 with PREFIX0.AP0.
    NRF_RADIO->BASE0 = config->base0;
    NRF_RADIO->PREFIX0 = config->prefix0;
    NRF_RADIO->TXADDRESS = 0; // transmit on logical address 0
    NRF_RADIO->RXADDRESSES = 1; // a bit mask, listen only to logical address 0

    // LFLEN=8 bits, S0LEN=0, S1LEN=0
    NRF_RADIO->PCNF0 = 0x00000008;
    // STATLEN=0, BALEN=4, ENDIAN=0 (little), WHITEEN=1
    NRF_RADIO->PCNF1 = 0x02040000 | config->max_payload;

    // Enable automatic 16bit CRC generation and checking, and configure how the CRC is calculated.
    NRF_RADIO->CRCCNF = RADIO_CRCCNF_LEN_Two;
    NRF_RADIO->CRCINIT = 0xFFFF;
    NRF_RADIO->CRCPOLY = 0x11021;

    // Set the start random value of the data whitening algorithm. This can be any non zero number.
    NRF_RADIO->DATAWHITEIV = 0x18;

    // Set the tx/rx packet buffer (must be in RAM).
    NRF_RADIO->PACKETPTR = (uint32_t)MP_STATE_PORT(radio_buf);

    // configure interrupts
    NRF_RADIO->INTENSET = 0x00000008;
    NVIC_SetPriority(RADIO_IRQn, 3);
    NVIC_ClearPendingIRQ(RADIO_IRQn);
    NVIC_EnableIRQ(RADIO_IRQn);

    NRF_RADIO->SHORTS |= RADIO_SHORTS_ADDRESS_RSSISTART_Msk;

    // enable receiver
    NRF_RADIO->EVENTS_READY = 0;
    NRF_RADIO->TASKS_RXEN = 1;
    while (NRF_RADIO->EVENTS_READY == 0) {
    }

    NRF_RADIO->EVENTS_END = 0;
    NRF_RADIO->TASKS_START = 1;
}

void microbit_radio_disable(void) {
    NVIC_DisableIRQ(RADIO_IRQn);
    NRF_RADIO->EVENTS_DISABLED = 0;
    NRF_RADIO->TASKS_DISABLE = 1;
    while (NRF_RADIO->EVENTS_DISABLED == 0) {
    }

    // free any old buffers
    if (MP_STATE_PORT(radio_buf) != NULL) {
        m_del(uint8_t, MP_STATE_PORT(radio_buf), rx_buf_end - MP_STATE_PORT(radio_buf));
        MP_STATE_PORT(radio_buf) = NULL;
    }
}

void microbit_radio_update_config(microbit_radio_config_t *config) {
    // disable radio
    NVIC_DisableIRQ(RADIO_IRQn);
    NRF_RADIO->EVENTS_DISABLED = 0;
    NRF_RADIO->TASKS_DISABLE = 1;
    while (NRF_RADIO->EVENTS_DISABLED == 0) {
    }

    // change state
    NRF_RADIO->TXPOWER = config->power_dbm;
    NRF_RADIO->FREQUENCY = config->channel;
    NRF_RADIO->MODE = config->data_rate;
    NRF_RADIO->BASE0 = config->base0;
    NRF_RADIO->PREFIX0 = config->prefix0;

    // need to set RXEN for FREQUENCY decision point
    NRF_RADIO->EVENTS_READY = 0;
    NRF_RADIO->TASKS_RXEN = 1;
    while (NRF_RADIO->EVENTS_READY == 0) {
    }

    // need to set START for BASE0 and PREFIX0 decision point
    NRF_RADIO->EVENTS_END = 0;
    NRF_RADIO->TASKS_START = 1;

    NVIC_ClearPendingIRQ(RADIO_IRQn);
    NVIC_EnableIRQ(RADIO_IRQn);
}

// This assumes the radio is enabled.
void microbit_radio_send(const void *buf, size_t len, const void *buf2, size_t len2) {
    // transmission will occur synchronously
    NVIC_DisableIRQ(RADIO_IRQn);

    // Turn off the transceiver.
    NRF_RADIO->EVENTS_DISABLED = 0;
    NRF_RADIO->TASKS_DISABLE = 1;
    while (NRF_RADIO->EVENTS_DISABLED == 0) {
    }

    // construct the packet
    // note: we must send from RAM
    size_t max_len = NRF_RADIO->PCNF1 & 0xff;
    if (len + len2 > max_len) {
        if (len > max_len) {
            len = max_len;
            len2 = 0;
        } else {
            len2 = max_len - len;
        }
    }
    MP_STATE_PORT(radio_buf)[0] = len + len2;
    memcpy(MP_STATE_PORT(radio_buf) + 1, buf, len);
    if (len2 != 0) {
        memcpy(MP_STATE_PORT(radio_buf) + 1 + len, buf2, len2);
    }

    // Turn on the transmitter, and wait for it to signal that it's ready to use.
    NRF_RADIO->EVENTS_READY = 0;
    NRF_RADIO->TASKS_TXEN = 1;
    while (NRF_RADIO->EVENTS_READY == 0) {
    }

    // Start transmission and wait for end of packet.
    NRF_RADIO->TASKS_START = 1;
    NRF_RADIO->EVENTS_END = 0;
    while (NRF_RADIO->EVENTS_END == 0) {
    }

    // Turn off the transmitter.
    NRF_RADIO->EVENTS_DISABLED = 0;
    NRF_RADIO->TASKS_DISABLE = 1;
    while (NRF_RADIO->EVENTS_DISABLED == 0) {
    }

    // Start listening for the next packet
    NRF_RADIO->EVENTS_READY = 0;
    NRF_RADIO->TASKS_RXEN = 1;
    while (NRF_RADIO->EVENTS_READY == 0) {
    }

    NRF_RADIO->EVENTS_END = 0;
    NRF_RADIO->TASKS_START = 1;

    NVIC_ClearPendingIRQ(RADIO_IRQn);
    NVIC_EnableIRQ(RADIO_IRQn);
}

const uint8_t *microbit_radio_peek(void) {
    // Disable the radio IRQ while we peek for packet.
    NVIC_DisableIRQ(RADIO_IRQn);

    // Get the pointer to the next waiting packet.
    const uint8_t *buf = MP_STATE_PORT(radio_buf) + (NRF_RADIO->PCNF1 & 0xff) + RADIO_PACKET_OVERHEAD; // skip tx buf

    // Return NULL if there are no packets waiting.
    if (rx_buf == buf) {
        buf = NULL;
    }

    // Re-enable the radio IRQ.
    NVIC_EnableIRQ(RADIO_IRQn);

    return buf;
}

void microbit_radio_pop(void) {
    // Disable the radio IRQ while we pop the packet.
    NVIC_DisableIRQ(RADIO_IRQn);

    // Get the pointer to the next packet, skipping the TX buffer at the start.
    uint8_t *buf = MP_STATE_PORT(radio_buf) + (NRF_RADIO->PCNF1 & 0xff) + RADIO_PACKET_OVERHEAD;

    if (rx_buf != buf) {
        // Copy all subsequent packets down over the first one.
        size_t len = buf[0];
        memmove(buf, buf + RADIO_PACKET_OVERHEAD + len, rx_buf - (buf + RADIO_PACKET_OVERHEAD + len));
        rx_buf -= RADIO_PACKET_OVERHEAD + len;
    }

    // Re-enable the radio IRQ.
    NVIC_EnableIRQ(RADIO_IRQn);
}

MP_REGISTER_ROOT_POINTER(uint8_t *radio_buf);
