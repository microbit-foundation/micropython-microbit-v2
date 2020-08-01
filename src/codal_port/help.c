/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2014 Damien P. George
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

#include "py/builtin.h"

const char microbit_help_text[] =
    "Welcome to MicroPython on the micro:bit!\n"
    "\n"
    "Try these commands:\n"
    "  display.scroll('Hello')\n"
    "  running_time()\n"
    "  sleep(1000)\n"
    "  button_a.is_pressed()\n"
    "What do these commands do? Can you improve them? HINT: use the up and down\n"
    "arrow keys to get your command history. Press the TAB key to auto-complete\n"
    "unfinished words (so 'di' becomes 'display' after you press TAB). These\n"
    "tricks save a lot of typing and look cool!\n"
    "\n"
    "Explore:\n"
    "Type 'help(something)' to find out about it. Type 'dir(something)' to see what\n"
    "it can do. Type 'dir()' to see what stuff is available. For goodness sake,\n"
    "don't type 'import this'.\n"
    "\n"
    "Control commands:\n"
    "  CTRL-C        -- stop a running program\n"
    "  CTRL-D        -- on a blank line, do a soft reset of the micro:bit\n"
    "  CTRL-E        -- enter paste mode, turning off auto-indent\n"
    "\n"
    "For a list of available modules, type help('modules')\n"
    "\n"
    "For more information about Python, visit: http://python.org/\n"
    "To find out about MicroPython, visit: http://micropython.org/\n"
    "Python/micro:bit documentation is here: https://microbit-micropython.readthedocs.io/\n"
;
