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
#include "py/smallint.h"
#include "drv_radio.h"

STATIC microbit_radio_config_t radio_config;

STATIC mp_obj_t mod_radio_reset(void);

STATIC void ensure_enabled(void) {
    if (MP_STATE_PORT(radio_buf) == NULL) {
        mp_raise_ValueError(MP_ERROR_TEXT("radio is not enabled"));
    }
}

STATIC mp_obj_t mod_radio___init__(void) {
    mod_radio_reset();
    microbit_radio_enable(&radio_config);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(mod_radio___init___obj, mod_radio___init__);

STATIC mp_obj_t mod_radio_reset(void) {
    radio_config.max_payload = MICROBIT_RADIO_DEFAULT_MAX_PAYLOAD;
    radio_config.queue_len = MICROBIT_RADIO_DEFAULT_QUEUE_LEN;
    radio_config.channel = MICROBIT_RADIO_DEFAULT_CHANNEL;
    radio_config.power_dbm = MICROBIT_RADIO_DEFAULT_POWER_DBM;
    radio_config.base0 = MICROBIT_RADIO_DEFAULT_BASE0;
    radio_config.prefix0 = MICROBIT_RADIO_DEFAULT_PREFIX0;
    radio_config.data_rate = MICROBIT_RADIO_DEFAULT_DATA_RATE;
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(mod_radio_reset_obj, mod_radio_reset);

STATIC mp_obj_t mod_radio_config(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    (void)pos_args; // unused

    if (n_args != 0) {
        mp_raise_TypeError(MP_ERROR_TEXT("arguments must be keywords"));
    }

    // make a copy of the radio state so we don't change anything if there are value errors
    microbit_radio_config_t new_config = radio_config;

    qstr arg_name = MP_QSTR_;
    for (size_t i = 0; i < kw_args->alloc; ++i) {
        if (MP_MAP_SLOT_IS_FILLED(kw_args, i)) {
            mp_int_t value = mp_obj_get_int_truncated(kw_args->table[i].value);
            arg_name = mp_obj_str_get_qstr(kw_args->table[i].key);
            switch (arg_name) {
                case MP_QSTR_length:
                    if (!(1 <= value && value <= 251)) {
                        goto value_error;
                    }
                    new_config.max_payload = value;
                    break;

                case MP_QSTR_queue:
                    if (!(1 <= value && value <= 254)) {
                        goto value_error;
                    }
                    new_config.queue_len = value;
                    break;

                case MP_QSTR_channel:
                    if (!(0 <= value && value <= MICROBIT_RADIO_MAX_CHANNEL)) {
                        goto value_error;
                    }
                    new_config.channel = value;
                    break;

                case MP_QSTR_power: {
                    if (!(0 <= value && value <= 7)) {
                        goto value_error;
                    }
                    static int8_t power_dbm_table[8] = {-30, -20, -16, -12, -8, -4, 0, 4};
                    new_config.power_dbm = power_dbm_table[value];
                    break;
                }

                case MP_QSTR_data_rate:
                    if (!(value == 2 /* allow 250K if the user really wants it */
                        || value == RADIO_MODE_MODE_Nrf_1Mbit
                        || value == RADIO_MODE_MODE_Nrf_2Mbit)) {
                        goto value_error;
                    }
                    new_config.data_rate = value;
                    break;

                case MP_QSTR_address:
                    new_config.base0 = value;
                    break;

                case MP_QSTR_group:
                    if (!(0 <= value && value <= 255)) {
                        goto value_error;
                    }
                    new_config.prefix0 = value;
                    break;

                default:
                    nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("unknown argument '%q'"), arg_name));
                    break;
            }
        }
    }

    // reconfigure the radio with the new state

    if (MP_STATE_PORT(radio_buf) == NULL) {
        // radio disabled, just copy state
        radio_config = new_config;
    } else {
        // radio eabled
        if (new_config.max_payload != radio_config.max_payload || new_config.queue_len != radio_config.queue_len) {
            // tx/rx buffer size changed which requires reallocating the buffers
            microbit_radio_disable();
            radio_config = new_config;
            microbit_radio_enable(&radio_config);
        } else {
            // only registers changed so make the changes go through efficiently
            radio_config = new_config;
            microbit_radio_update_config(&radio_config);
        }
    }

    return mp_const_none;

value_error:
    nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("value out of range for argument '%q'"), arg_name));
}
MP_DEFINE_CONST_FUN_OBJ_KW(mod_radio_config_obj, 0, mod_radio_config);

STATIC mp_obj_t mod_radio_on(void) {
    microbit_radio_enable(&radio_config);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(mod_radio_on_obj, mod_radio_on);

STATIC mp_obj_t mod_radio_off(void) {
    microbit_radio_disable();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_0(mod_radio_off_obj, mod_radio_off);

STATIC mp_obj_t mod_radio_send_bytes(mp_obj_t buf_in) {
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_READ);
    ensure_enabled();
    microbit_radio_send(bufinfo.buf, bufinfo.len, NULL, 0);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(mod_radio_send_bytes_obj, mod_radio_send_bytes);

STATIC mp_obj_t mod_radio_receive_bytes(void) {
    ensure_enabled();
    const uint8_t *buf = microbit_radio_peek();
    if (buf == NULL) {
        return mp_const_none;
    } else {
        mp_obj_t ret = mp_obj_new_bytes(buf + 1, buf[0]);
        microbit_radio_pop();
        return ret;
    }
}
MP_DEFINE_CONST_FUN_OBJ_0(mod_radio_receive_bytes_obj, mod_radio_receive_bytes);

STATIC mp_obj_t mod_radio_send(mp_obj_t buf_in) {
    mp_uint_t len;
    const char *data = mp_obj_str_get_data(buf_in, &len);
    ensure_enabled();
    microbit_radio_send("\x01\x00\x01", 3, data, len);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(mod_radio_send_obj, mod_radio_send);

STATIC mp_obj_t mod_radio_receive(void) {
    ensure_enabled();
    const uint8_t *buf = microbit_radio_peek();
    if (buf == NULL) {
        return mp_const_none;
    } else {
        // Verify header has the correct values for an encoded string object.
        if (!(buf[0] >= 3 && buf[1] == 1 && buf[2] == 0 && buf[3] == 1)) {
            microbit_radio_pop();
            mp_raise_ValueError(MP_ERROR_TEXT("received packet is not a string"));
        }
        mp_obj_t ret = mp_obj_new_str((const char *)buf + 4, buf[0] - 3);
        microbit_radio_pop();
        return ret;
    }
}
MP_DEFINE_CONST_FUN_OBJ_0(mod_radio_receive_obj, mod_radio_receive);

STATIC mp_obj_t mod_radio_receive_bytes_into(mp_obj_t buf_in) {
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_WRITE);
    ensure_enabled();
    const uint8_t *buf = microbit_radio_peek();
    if (buf == NULL) {
        return mp_const_none;
    } else {
        size_t len = buf[0];
        memcpy(bufinfo.buf, buf + 1, MIN(bufinfo.len, len));
        microbit_radio_pop();
        return MP_OBJ_NEW_SMALL_INT(len);
    }
}
MP_DEFINE_CONST_FUN_OBJ_1(mod_radio_receive_bytes_into_obj, mod_radio_receive_bytes_into);

STATIC mp_obj_t mod_radio_receive_full(void) {
    ensure_enabled();
    const uint8_t *buf = microbit_radio_peek();
    if (buf == NULL) {
        return mp_const_none;
    } else {
        size_t len = buf[0];
        int rssi = -buf[1 + len];
        uint32_t timestamp_us = buf[1 + len + 1]
            | buf[1 + len + 2] << 8
            | buf[1 + len + 3] << 16
            | buf[1 + len + 4] << 24;
        mp_obj_t tuple[3] = {
            mp_obj_new_bytes(buf + 1, len),
            MP_OBJ_NEW_SMALL_INT(rssi),
            MP_OBJ_NEW_SMALL_INT(timestamp_us & (MICROPY_PY_TIME_TICKS_PERIOD - 1))
        };
        microbit_radio_pop();
        return mp_obj_new_tuple(3, tuple);
    }
}
MP_DEFINE_CONST_FUN_OBJ_0(mod_radio_receive_full_obj, mod_radio_receive_full);

STATIC const mp_map_elem_t radio_module_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_radio) },
    { MP_OBJ_NEW_QSTR(MP_QSTR___init__), (mp_obj_t)&mod_radio___init___obj },

    { MP_OBJ_NEW_QSTR(MP_QSTR_reset), (mp_obj_t)&mod_radio_reset_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_config), (mp_obj_t)&mod_radio_config_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_on), (mp_obj_t)&mod_radio_on_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_off), (mp_obj_t)&mod_radio_off_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_send_bytes), (mp_obj_t)&mod_radio_send_bytes_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_receive_bytes), (mp_obj_t)&mod_radio_receive_bytes_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_send), (mp_obj_t)&mod_radio_send_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_receive), (mp_obj_t)&mod_radio_receive_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_receive_bytes_into), (mp_obj_t)&mod_radio_receive_bytes_into_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_receive_full), (mp_obj_t)&mod_radio_receive_full_obj },

    // A rate of 250Kbit is physically supported by the nRF52 but it is deprecated,
    // so don't provide the constant to the Python user.  They can still select this
    // rate by using a value of "2" if necessary to communicate with a micro:bit v1.
    //{ MP_OBJ_NEW_QSTR(MP_QSTR_RATE_250KBIT), MP_OBJ_NEW_SMALL_INT(RADIO_MODE_MODE_Nrf_250Kbit) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_RATE_1MBIT), MP_OBJ_NEW_SMALL_INT(RADIO_MODE_MODE_Nrf_1Mbit) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_RATE_2MBIT), MP_OBJ_NEW_SMALL_INT(RADIO_MODE_MODE_Nrf_2Mbit) },
};

STATIC MP_DEFINE_CONST_DICT(radio_module_globals, radio_module_globals_table);

const mp_obj_module_t radio_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&radio_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_radio, radio_module);
