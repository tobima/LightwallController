LightwallController
===================

System
------

Based on the following hardware:
- ARM-Cortex-M4 STM32F407
- STM32F4DIS-BB (Base Board with Ethernet, RS232, MicroSD card slot)

Adaption to the Hardware could be found in the `Hardware folder` ;-)


Software
--------

The manual to setup the development environment can be found at:
http://www.ccc-mannheim.de/wiki/Chibios

After the first download the following script must be executed:
```
InitSubModules.sh
```
Building and flashing:
```
cd Firmware
make
make flash (USB must be reconnected, if flashing failed)
```