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

#include "py/obj.h"
#include "py/objtuple.h"
#include "py/objstr.h"
#include "ports/nrf/modules/uos/microbitfs.h"

// Include MicroPython and micro:bit version information.
#include "genhdr/mpversion.h"
#include "genhdr/microbitversion.h"

#define MICROBIT_VERSION \
    "micro:bit v" MICROBIT_RELEASE "+" MICROBIT_GIT_HASH " on " MICROBIT_BUILD_DATE \
    "; MicroPython " MICROPY_GIT_TAG " on " MICROPY_BUILD_DATE

const char microbit_release_string[] = MICROBIT_RELEASE;
const char microbit_version_string[] = MICROBIT_VERSION;

STATIC const qstr os_uname_info_fields[] = {
    MP_QSTR_sysname, MP_QSTR_nodename,
    MP_QSTR_release, MP_QSTR_version, MP_QSTR_machine
};
STATIC const MP_DEFINE_STR_OBJ(os_uname_info_sysname_obj, MICROPY_PY_SYS_PLATFORM);
STATIC const MP_DEFINE_STR_OBJ(os_uname_info_nodename_obj, MICROPY_PY_SYS_PLATFORM);
STATIC const MP_DEFINE_STR_OBJ(os_uname_info_release_obj, microbit_release_string);
STATIC const MP_DEFINE_STR_OBJ(os_uname_info_version_obj, microbit_version_string);
STATIC const MP_DEFINE_STR_OBJ(os_uname_info_machine_obj, MICROBIT_BOARD_NAME " with " MICROPY_HW_MCU_NAME);

STATIC MP_DEFINE_ATTRTUPLE(
    os_uname_info_obj,
    os_uname_info_fields,
    5,
    MP_ROM_PTR(&os_uname_info_sysname_obj),
    MP_ROM_PTR(&os_uname_info_nodename_obj),
    MP_ROM_PTR(&os_uname_info_release_obj),
    MP_ROM_PTR(&os_uname_info_version_obj),
    MP_ROM_PTR(&os_uname_info_machine_obj)
);

STATIC mp_obj_t os_uname(void) {
    return MP_OBJ_FROM_PTR(&os_uname_info_obj);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(os_uname_obj, os_uname);

STATIC mp_obj_t os_size(mp_obj_t filename) {
    mp_obj_tuple_t *tuple = MP_OBJ_TO_PTR(uos_mbfs_stat_obj.fun._1(filename));
    return tuple->items[6];
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(os_size_obj, os_size);

STATIC const mp_rom_map_elem_t os_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_os) },

    { MP_ROM_QSTR(MP_QSTR_uname), MP_ROM_PTR(&os_uname_obj) },

    { MP_ROM_QSTR(MP_QSTR_listdir), MP_ROM_PTR(&uos_mbfs_listdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_ilistdir), MP_ROM_PTR(&uos_mbfs_ilistdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_remove), MP_ROM_PTR(&uos_mbfs_remove_obj) },
    { MP_ROM_QSTR(MP_QSTR_stat), MP_ROM_PTR(&uos_mbfs_stat_obj) },

    // micro:bit v1 specific
    { MP_ROM_QSTR(MP_QSTR_size), MP_ROM_PTR(&os_size_obj) },
};
STATIC MP_DEFINE_CONST_DICT(os_module_globals, os_module_globals_table);

const mp_obj_module_t os_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&os_module_globals,
};
