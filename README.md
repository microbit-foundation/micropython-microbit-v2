MicroPython on the Calliope mini via CODAL based on the port of microbit
=========================================================================

This is a port of MicroPython to the Calliope mini which uses the CODAL as the
underlying target platform.

Setting up
----------

After cloning this repository update the submodules:

    $ git submodule update --init

Then build the MicroPython cross-compiler:

    $ make -C lib/micropython/mpy-cross

Note: you may need to add to the line above `PYTHON=python` (or some variant
thereof) to select your Python executable (this defaults to `python3`); for
example:

    $ make -C lib/micropython/mpy-cross PYTHON=python

Building and running
--------------------

After setting up, go to the `src/` directory and build
(as above, you may need to add `PYTHON=python` to the `make` command):

    $ cd src
    $ make

That will build both `libmicropython.a` (from source in `src/codal_port/`) and
the CODAL app (from source in `src/codal_app/`).  The resulting firmware will be
`MINI.hex` in the `src/` directory which can be copied to the Calliope mini.

Once the firmware is on the device there will appear a REPL prompt on the serial
port.  To test it you can run:

    >>> display.show(Image.HAPPY)
    >>> audio.play(Sound.HAPPY)
    
Code of Conduct (from microbit)
--------------------------------

Trust, partnership, simplicity and passion are our core values we live and 
breathe in our daily work life and within our projects. Our open-source 
projects are no exception. We have an active community which spans the globe 
and we welcome and encourage participation and contributions to our projects 
by everyone. We work to foster a positive, open, inclusive and supportive 
environment and trust that our community respects the micro:bit code of conduct. 

Please see our [code of conduct](https://microbit.org/safeguarding/) which 
outlines our expectations for all those that participate in our community and 
details on how to report any concerns and what would happen should breaches occur.
