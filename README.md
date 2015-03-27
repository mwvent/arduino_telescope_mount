GEM Telescope Mount Control with an Arduino Mega
------------------------------------------------
The goal of this project is to enable conversion of a non-motorised Equatorial Telescope mount to an goto capable mount with tracking.

I started this project shortly after buying and trying out a simple type tracking motor for my mount. It was an absolute nightmare to use as you had to point the telscope at the target object and then somehow fit the motor to the shaft in the dark while not loosing sight of the object. As impossible as that was, if you were lucky enougth to pull it off you would loose your target very quickly anyway. With just a constant motor power that you set with an anaolouge dial and no controller to read how much the shaft was actually moving the object would quickly be lost while the motor went too slow at some points and too fast at others.
After giving up hope I dismantled the unit and with an encoder I was lucky enougth to find out of an old machine, and my arduino I created a simple feedback loop which eventually grew into the project I have here now.

This is by no means the only Arduino telescope driver out there, what I think makes this one different is the flexibility in changing the design by making the code very modular and a focus on more affordable components.

The software provides the following features
   Control of the mounts axis using DC motors with encoder feedback
   Largley LX200 compatible interface on the Serial ports of the Arduino for PC control
   A user interface using an LCD and Wii nunchuck connected to the I2C Bus, this is optional/easily changed see notes below
   Tracking
And I am working on
   Periodic error correction for RA Axis encoders that are indexed
   An INDILib driver for finer control 

This code uses the following hardware

Required
--------
Arduino Mega 2560
Adafruit motor sheild* (see below for other options)
DS1307 Real Time Clock on I2C Bus
2 DC Motors connected to the mounts 2 axis
2 Encoders connected to the mounts 2 axis (or integrated with the motor)

Hardware that will is currentley coded in but will be made optional in a future update
--------------------------------------------------------------------------------------
LCD Screen with I2C Backpack for communications* (see below for other options)
Ninendo Wii nunchuck* (see below for other options)

Optional
--------
DC Motor conencted to telescope focuser
BC06 bluetooth to serial module connected to SPI-1


Getting started
---------------
- Hookup your motors to the adafruit motor sheild
- Download the adafruit motor sheild library for your Arduino and write a small sketch to drive the motors in various directions with your telescope mount fully loaded. Note what the minimum speed setting is needed to get the motors moving
- Try higher speeds and check the H-Bridge chips on your motor sheild for overheating. Newer boards have overheating protection but older boards like mine can be smoked by running the motor at full power.
- Hookup your encoders, the pins are
  A8 & A9 - RA Encoder A/B
  A10 & A11 - Dec Encoder A/B
  A12, if your RA Encoder has a z-index hook it up here. An RA Encoder with an index that is coupled to the RA shaft will enable Periodic error correction in later editions of this code
- Different pin configs will need to be hard coded into encoders.cpp, because interrupts are fired with PORTK (pins A8-1A15) the code here needs to focus on execution speed rather than clean configurable code
- Hookup the DC1307, LCD Screen and Wii nunchuck. If you want to drive the telescope purley from the serial port with LX200 and not use the LCD / Wii controller  do this by commenting out the lines UI_c* UI; - UI = new UI_c(mount, DS1307_RTC, EEPROMHandler); and UI->update();
- Enter in the minimum and maximum motor speeds into settings.h, you will also need to enter the PPR (pulses per revolution) for each encoder into settings.h and the amount of gear teeth on your axis (or rather the amount of revolutions the worm gear would make to fully rotate the axis). Finally update settings.h with the motor ports (on the adafruit shield) you connected each motor to.

Modifications for use with other hardware
-----------------------------------------
The adafruit motor board was a great option when I started out for driving the DC motors. However as I learnt the hard way a few times the chips are easily burnt out not to mention there are a ton of other great options for driving motors.
Luckily there is very little that needs to be changed to use other motor drivers - all calls that need to be changed are in axis.cpp/h

In the constructor function for each axis class the following line creates the motor class (from the adafruit library) - this can be removed/replaced
motor = new AF_DCMotor(motorport, MOTOR12_1KHZ);
The only functions that will need changing are setMotorSpeed (which takes an 8bit 0-255 speed value), stopMotor, and the 4 runMotorx functions. No other code changes will be necessary to run a different motor drive.

To use a different joystick see joystickcontrol.cpp/h. To modify this strip out the nunchuck/I2C code. All this class needs to do is provide xevent functions and the filteredx / filteredy values, the newDataAvailibleEvent value and the update function. Events are 'latching' - so for example in your update function when you read the joysticks state, if you find the user has pushed the RIGHT button (or moved the analog stick above a threshold you consider to be RIGHT) you set a value to indicate this has happened and set newDataAvailibleEvent to true.
When the UI class queries the joystick class it will call right_event() to see if that event has happened. The right event function should check the value the update function set and return true if right was press. Crucially the event should then be cleared by the right_event() function. Do this for all events and you have an alterative joystick class.

Of course you may not feel the need to attach a joystick/LCD to your mount at all. Most people are quite happy controlling the mount with a laptop or smartphone/tablet. To disable the LCD and joystick alltogether comment out the lines UI_c* UI; - UI = new UI_c(mount, DS1307_RTC, EEPROMHandler); and UI->update(); from ArduinoGEMDriver.ino

The DS1307 realtime clock is another hard-coded bit of hardware I am looking at an easy way to opt out of in the code. To be fair these can be got very cheaply though and it is a worthwhile addition to the project. A battery backed up one can ensure the arudino has some non-volatile memory that it can write to very often (the EEPROM built into the Arduino can be burnt out very quickly)


A video of this setup in action
https://www.youtube.com/watch?v=mgYvCZ1bOII


