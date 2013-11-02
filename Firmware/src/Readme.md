# LightwallController - Firmware #

This system is built around the `ARM-Cortex-M4 STM32F407` and `Chibios`.


## Shell commands ##
- mem - displays memory usage
- threads - all threads on the device
- ifconfig - ethernet configuration
- cat - content of a given file

The shell can be accessed via RS232 (UART6) or the SubD-port of the base board.

Source locations are:
```
src\cmd
```

## Library modules ##

- ini	(parsing ini files)
- conf	(read network configuration; needs ```ini```)
- dmx	(Send DMX universe via /Hardware/DMXextension)
- fullcircle	(Server handling dynamic fullcircle frames from network)
- web	(Tiny webserver)
