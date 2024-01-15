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
#include "microbithal.h"
#include "MicroBitDevice.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

extern "C" void microbit_hal_level_detector_callback(int);

static void level_detector_event_handler(Event evt) {
    microbit_hal_level_detector_callback(evt.value);
}

class MyStreamRecording : public DataSink
{
    public:
    DataSource &upStream;

    public:
    uint8_t *dest;
    size_t *dest_pos_ptr;
    size_t dest_max;
    bool request_stop;

    MyStreamRecording(DataSource &source);
    virtual ~MyStreamRecording();

    virtual int pullRequest();
};

MyStreamRecording::MyStreamRecording( DataSource &source ) : upStream( source )
{
}

MyStreamRecording::~MyStreamRecording()
{
}

int MyStreamRecording::pullRequest()
{
    ManagedBuffer data = this->upStream.pull();

    size_t n = MIN((size_t)data.length(), this->dest_max - *this->dest_pos_ptr);
    if (n == 0 || this->request_stop) {
        this->upStream.disconnect();
        this->request_stop = false;
    } else {
        // Copy and convert signed 8-bit to unsigned 8-bit data.
        const uint8_t *src = data.getBytes();
        uint8_t *dest = this->dest + *this->dest_pos_ptr;
        for (size_t i = 0; i < n; ++i) {
            *dest++ = *src++ + 128;
        }
        *this->dest_pos_ptr += n;
    }

    return DEVICE_OK;
}

static MyStreamRecording *recording = NULL;
static SplitterChannel *splitterChannel = NULL;

extern "C" {

static bool microphone_init_done = false;

void microbit_hal_microphone_init(void) {
    if (!microphone_init_done) {
        microphone_init_done = true;
        uBit.audio.levelSPL->setUnit(LEVEL_DETECTOR_SPL_8BIT);
        uBit.messageBus.listen(DEVICE_ID_SYSTEM_LEVEL_DETECTOR, DEVICE_EVT_ANY, level_detector_event_handler);
    }
}

void microbit_hal_microphone_set_threshold(int kind, int value) {
    if (kind == MICROBIT_HAL_MICROPHONE_SET_THRESHOLD_LOW) {
        uBit.audio.levelSPL->setLowThreshold(value);
    } else {
        uBit.audio.levelSPL->setHighThreshold(value);
    }
}

int microbit_hal_microphone_get_level(void) {
    int value = uBit.audio.levelSPL->getValue();
    return value;
}

float microbit_hal_microphone_get_level_db(void) {
    uBit.audio.levelSPL->setUnit(LEVEL_DETECTOR_SPL_DB);
    float value = uBit.audio.levelSPL->getValue();
    uBit.audio.levelSPL->setUnit(LEVEL_DETECTOR_SPL_8BIT);
    return value;
}

void microbit_hal_microphone_start_recording(uint8_t *buf, size_t max_len, size_t *cur_len, int rate) {
    if (splitterChannel == NULL) {
        splitterChannel = uBit.audio.splitter->createChannel();
        splitterChannel->setFormat(DATASTREAM_FORMAT_8BIT_UNSIGNED);
        // Increase sample period to 64us, so we can get our desired rate.
        splitterChannel->requestSampleRate(1000000 / 64);
    }
    splitterChannel->requestSampleRate(rate);

    if (recording == NULL) {
        recording = new MyStreamRecording(*splitterChannel);
    } else {
        if (microbit_hal_microphone_is_recording()) {
            microbit_hal_microphone_stop_recording();
            while (microbit_hal_microphone_is_recording()) {
                microbit_hal_idle();
            }
        }
    }

    recording->dest = buf;
    recording->dest_pos_ptr = cur_len;
    *recording->dest_pos_ptr = 0;
    recording->dest_max = max_len;
    recording->request_stop = false;

    splitterChannel->connect(*recording);
}

bool microbit_hal_microphone_is_recording(void) {
    return recording != NULL && splitterChannel->isConnected();
}

void microbit_hal_microphone_stop_recording(void) {
    if (recording != NULL) {
        recording->request_stop = true;
    }
}

}
