COVID button
============

In the context of COVID vigilence, the current project aims to create a
contact-free button to interact with other projects, for example to be used as
gamepad for the Pong1D project. It is based on the ST VL53L1X laser
long-distance ranging time-of-flight sensor.


Hardware
--------

STM32F411 nucleo board + VL53L1X-SATEL satellite board + two pull-up resistors

Wire SCL (2), SDA (4), GND (6) and  VCC (5) of the satellite board to the
corresponding pins of the nucleo board (The I2C used is the one labelled on the
board). Do not forget to add pull-up resistors on SDA and SCL.

Outputs:

PA5: 3V3 = touch ; 0V = no touch

PA6: 0V = touch ; OPEN = no touch

Compile
-------

This is a platformio projet. To compile and flash: `pio run -t upload`.

You can install platformio with `pip install platformio`. Note that currently
(2020-07) you have to update the content of
~/.platformio/packages/framework-libopencm3
with the content of the libopencm3 repository as the official platformio package
distributes outdated code.
