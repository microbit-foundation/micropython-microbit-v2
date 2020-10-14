MicroPython on the micro:bit via CODAL
======================================

This is a port of MicroPython to the micro:bit which uses the CODAL as the
underlying target platform.

Setting up
----------

After cloning this repository update the submodules:

    $ git submodule update --init

Currently lib/micropython is the only submodule registered with git.  The codal
sub-repository and its dependencies must be explicitly cloned using:

    $ git clone https://github.com/lancaster-university/codal.git lib/codal
    $ git clone --recurse-submodules https://github.com/lancaster-university/codal-core.git lib/codal/libraries/codal-core
    $ git clone --recurse-submodules https://github.com/lancaster-university/codal-nrf52.git lib/codal/libraries/codal-nrf52
    $ git clone --recurse-submodules https://github.com/microbit-foundation/codal-microbit-nrf5sdk.git lib/codal/libraries/codal-microbit-nrf5sdk
    $ git clone --recurse-submodules https://github.com/lancaster-university/codal-microbit-v2.git lib/codal/libraries/codal-microbit-v2

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
