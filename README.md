GEM Telescope Mount Control with an Arduino Mega

This project uses DC motors to drive the telescope axis - ensure to use motors with high gearing.
This controller exposes an LX200 compatible interface on the Serial port (and SPI-1 if you wish to connect another serial interface such as a BC06 bluetooth to serial module) 
Sidereal tracking begins as soon as the mount is turned on


This code uses the following hardware

Required
--------
Arduino Mega 2560
Adafruit motor sheild
DS1307 Real Time Clock on I2C Bus
2 DC Motors connected to the mounts 2 axis
2 Encoders connected to the mounts 2 axis

Hardware that will is currentley coded in but will be made optional in a future update
--------------------------------------------------------------------------------------
LCD Screen with I2C Backpack for communications
Ninendo Wii nunchuck (directley coupled to I2C)

Optional
--------
DC Motor conencted to telescope focuser
BC06 bluetooth to serial module connected to SPI-1


Getting started
- Hookup your motors to the adafruit motor sheild
- Download the adafruit motor sheild library for your Arduino and write a small sketch to drive the motors in various directions with your telescope mount fully loaded. Note what the minimum speed setting is needed to get the motors moving
- Try higher speeds and check the H-Bridge chips on your motor sheild for overheating. Newer boards have overheating protection but older boards like mine can be smoked by running the motor at full power.
- Hookup your encoders, the pins are
  A8 & A9 - RA Encoder A/B
  A10 & A11 - Dec Encoder A/B
  A12, if your RA Encoder has a z-index hook it up here. An RA Encoder with an index that is coupled to the RA shaft will enable Periodic error correction in later editions of this code
  - Different pin configs will need to be hard coded into encoders.cpp, because interrupts are fired with PORTK (pins A8-1A15) the code here needs to focus on execution speed rather than clean configurable code
- Hookup the DC1307, LCD Screen and Wii nunchuck. If you want to drive the telescope purley from the serial port with LX200 and not use the LCD / Wii controller you *may* be able to do this by commenting out the lines UI_c* UI; - UI = new UI_c(mount, DS1307_RTC, EEPROMHandler); and UI->update(); - I need to test this though
- Enter in the minimum and maximum motor speeds into settings.h, you will also need to enter the PPR (pulses per revolution) for each encoder into settings.h and the amount of gear teeth on your axis (or rather the amount of revolutions the worm gear would make to fully rotate the axis). Finally update settings.h with the motor ports (on the adafruit shield) you connected each motor to.


A video of this setup in action
https://www.youtube.com/watch?v=mgYvCZ1bOII


