MicroPython on the micro:bit via CODAL
======================================

This is a port of MicroPython to the micro:bit which uses the CODAL as the
underlying target platform.

Setting up
----------

After cloning this repository update the submodules:

    $ git submodule update --init

Then build the MicroPython cross-compiler:

    $ make -C lib/micropython/mpy-cross

Building and running
--------------------

After setting up, go to the `src/` directory and build:

    $ cd src
    $ make

That will build both `libmicropython.a` (from source in `src/codal_port/`) and
the CODAL app (from source in `src/codal_app/`).  The resulting firmware will be
`MICROBIT.hex` in the `src/` directory which can be copied to the micro:bit.

Once the firmware is on the device there will appear a REPL prompt on the serial
port.  To test it you can run:

    >>> display.show(Image.HAPPY)
    >>> audio.play(Sound.HAPPY)
