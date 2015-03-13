CH_board_element14_STM32F4DIS-BB
================================

Board configuration for STM32F4DIS-BB board from element14

* USBUART
* Debug LED: PA8
* USB UART
* WS2811 on PB0

## Integrated from Submodule into Chibios-Project


1. Delete the relevant 'Firmware/board' from the `.gitmodules` file.
2. Stage the .gitmodules changes `git add .gitmodules`
3. Delete the relevant section from `.git/config`.
4. Run `git rm --cached Firmware/board` (no trailing slash).
5. Run `rm -rf .git/modules/Firmware/board/`
6. Commit git commit -m "Removed submodule Firmware/board"
7. Delete the now untracked submodule files `rm -rf Firmware/board`

Source: 
http://stackoverflow.com/questions/1260748/how-do-i-remove-a-git-submodule/1260982#1260982

