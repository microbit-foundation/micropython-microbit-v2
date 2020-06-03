MicroPython on the micro:bit via CODAL
======================================

This is a port of MicroPython to the micro:bit which uses the CODAL as the
underlying target platform.

Setting up
----------

After cloning this repository update the submodules:

    $ git submodule update --init

Currently lib/micropython is the only submodule registered with git.  The codal
sub-repositoriy must be explicitly cloned using:

    $ git clone https://github.com/microbit-foundation/codal.git lib/codal

This may require setting/passing extra authentication information.  You must
then follow the instructions for the codal repository to fetch its
dependencies.  They go in lib/codal/libraries and are codal-core, codal-nrf52
and codal-microbit.

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
