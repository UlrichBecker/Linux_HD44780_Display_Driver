# Linux device-driver for alpanumeric HD44780 compatible displays

At first the disadvantage:
- This module consumes at least 7 GPIO connections.

Advantages:
- HD44780 compatible displays are direct contactable by the linux host-device.
  That means, no additional (active) hardware necessary. (Except some pullup resistors.)
- GPIO connections free selectable by devicetree or device-tree overlay.
- Compilable as single device-driver "anLcd.ko" or as kernel-build-in.
- Using is independend of programming-languages. That means, no extra libraries for
  user-applications necessary.
- Support of some terminal escape sequences. e.g.:
... Cursor positioning (gotoxy)
... Cursor on/off
... Clear screen
... Clear line
- Automatic terminal-like scroll-up (if desired).
- Control of reset, scroll-up and scroll-down by ioctl().
- Ability of read-back.
- Driver status readable by process file system, e.g.: "cat /proc/driver/anLcd".
- HD44780 CG-RAM for special characters programmable by ioctl();

Base source-code in (./src ) is also suitable for AVR-microcontroller projects
e.g. for Arduino.

# Compiling
1) Go in the sub-directory ./ksrc
2) Type "make all" (builds the kernel module)
3) Type "make blob" (builds the device-tree overlay)

**First example**
```
echo "Hello world!" > /dev/anLcd
```
## Esc-sequences
Cursor on:
```
# printf "\e[?25h"  > /dev/anLcd
```
Cursor off:
```
# printf "\e[?25l"  > /dev/anLcd
```
Example for gotoxy(5,1):
```
# printf "\e[1;5H"  > /dev/anLcd
```
Example for clear-screen:
```
# printf "\e[H"  > /dev/anLcd
```


**IOCTL- commands**

The driver supports some ioctl-commands e.g. for the C- function ```ioctl()```.
This ioctl- commands are defined in the header- file ```./include/linux/an_disp_ioctl.h```

For quick tests or shell scripts you can use a bash variant of ioctl.

**Note:** 
This bash variant of ioctl is not a official bash-command but you can obtain this in the following repository:
https://github.com/UlrichBecker/ioctl4bash

By the process- file-system you can quickly obtain the related hex-number of
each defined ioctl- command.
```
# cat /proc/driver/anLcd 
```
```
anLcd 20x4 Version: 0.1

GPIO 17: rs = low
GPIO 27: rw = high
GPIO 22: en = low
GPIO 05: d4 = high
GPIO 06: d5 = high
GPIO 13: d6 = high
GPIO 26: d7 = high

Valid commands for ioctl():
AN_DISPLAY_IOC_RESET:   0x00006400
AN_DISPLAY_IOC_SCROLL_UP:       0x00006401
AN_DISPLAY_IOC_SCROLL_DOWN:     0x00006402
AN_DISPLAY_IOC_LOAD_DEFAULT_CGRAM:      0x00006407
AN_DISPLAY_IOC_WRITE_CGRAM:     0x40096408
AN_DISPLAY_IOC_AUTOSCROLL_ON:   0x00006403
AN_DISPLAY_IOC_AUTOSCROLL_OFF:  0x00006404
AN_DISPLAY_IOC_OFF:     0x00006406
AN_DISPLAY_IOC_ON:      0x00006405

Auto scroll: enabled
```
**Bash example for writing and displaying a self made character in CG-RAM:**
```
# printf "\x01\x01\x03\x07\x00\x00\x07\x03\x01" | ioctl -p=16 /dev/anLcd 0x40096408
# echo -e "My special: \x01"  > /dev/anLcd
```
**Note:** The first byte is the address byte of the CG-RAM (here address 0x01) followed by the 8 pattern-bytes.

