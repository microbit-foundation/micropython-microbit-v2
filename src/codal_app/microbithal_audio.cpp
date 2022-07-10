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

class AudioSource : public DataSource {
public:
    bool started;
    DataSink *sink;
    ManagedBuffer buf;
    void (*callback)(void);

    AudioSource()
        : started(false) {
    }

    virtual ManagedBuffer pull() {
        callback();
        return buf;
    }
    virtual void connect(DataSink& sink_in) {
        sink = &sink_in;
    }
    virtual void disconnect() {
    }
    virtual int getFormat() {
        return DATASTREAM_FORMAT_8BIT_UNSIGNED;
    }
};

static AudioSource data_source;
static AudioSource speech_source;

extern "C" {

#include "microbithal.h"

static uint8_t sound_synth_active_count = 0;

void microbit_hal_audio_select_pin(int pin) {
    if (pin < 0) {
        uBit.audio.setPinEnabled(false);
    } else {
        uBit.audio.setPinEnabled(true);
        uBit.audio.setPin(*pin_obj[pin]);
    }
}

void microbit_hal_audio_select_speaker(bool enable) {
    uBit.audio.setSpeakerEnabled(enable);
}

// Input value has range 0-255 inclusive.
void microbit_hal_audio_set_volume(int value) {
    if (value >= 255) {
        uBit.audio.setVolume(128);
    } else {
        uBit.audio.setVolume(value / 2);
    }
}

void microbit_hal_sound_synth_callback(int event) {
    if (event == DEVICE_SOUND_EMOJI_SYNTHESIZER_EVT_DONE) {
        --sound_synth_active_count;
    }
}

bool microbit_hal_audio_is_expression_active(void) {
    return sound_synth_active_count > 0;
}

void microbit_hal_audio_play_expression(const char *expr) {
    ++sound_synth_active_count;
    uBit.audio.soundExpressions.stop();

    // `expr` can be a built-in expression name, or expression data.
    // If it's expression data this method parses the data and stores
    // it in another buffer ready to play.  So `expr` does not need
    // to live for the duration of the playing.
    uBit.audio.soundExpressions.playAsync(expr);
}

void microbit_hal_audio_stop_expression(void) {
    uBit.audio.soundExpressions.stop();
}

void microbit_hal_audio_init(uint32_t sample_rate) {
    if (!data_source.started) {
        MicroBitAudio::requestActivation();
        data_source.started = true;
        data_source.callback = microbit_hal_audio_ready_callback;
        uBit.audio.mixer.addChannel(data_source, sample_rate, 255);
    }
}

void microbit_hal_audio_write_data(const uint8_t *buf, size_t num_samples) {
    if ((size_t)data_source.buf.length() != num_samples) {
        data_source.buf = ManagedBuffer(num_samples);
    }
    memcpy(data_source.buf.getBytes(), buf, num_samples);
    data_source.sink->pullRequest();
}

void microbit_hal_audio_speech_init(uint32_t sample_rate) {
    if (!speech_source.started) {
        MicroBitAudio::requestActivation();
        speech_source.started = true;
        speech_source.callback = microbit_hal_audio_speech_ready_callback;
        uBit.audio.mixer.addChannel(speech_source, sample_rate, 255);
    }
}

void microbit_hal_audio_speech_write_data(const uint8_t *buf, size_t num_samples) {
    if ((size_t)speech_source.buf.length() != num_samples) {
        speech_source.buf = ManagedBuffer(num_samples);
    }
    memcpy(speech_source.buf.getBytes(), buf, num_samples);
    speech_source.sink->pullRequest();
}

}
