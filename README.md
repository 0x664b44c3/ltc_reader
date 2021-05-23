# ltc_reader
A linear timecode reader module for AVR-microcontrollers

## Functionality

 * Decodes the biphase mark code
 * scans for the sync symbol in the bitstream
 * decodes the LTC into a struct and sets a flag bit once a frame is complete

## Requirements
 * One freerunning 16 bit timer with available ICP pin
 * MCU that is fast enaugh to do the decoding
 * external comparator circuit (see attached reference schematic) or software comparator using adc.

## Features not implemented
 * true flywheel code (a minimalistic implementation is included)
 * varispeed

## Porting

This code should run on basically any MCU out there given it is fast enaugh. LTC hold 80 bits per frame so one input signal edge may occour every 1 / (fps * 80 * 2) seconds. Which is roughly every 0.2 milliseconds at 30 FPS.
On the AVR platform this is using the input capture feature of Timer 1 (16 bit) to perform the first stage of decoding
Ports to other platforms are welcome.
