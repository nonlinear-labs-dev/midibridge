// version history :
//  1.02 : initial (with separate USB IDs/names for HS and FS port)
//  1.03 : unified USB IDs/names for HS and FS port for production version
//  1.04 : bugfix : suspend/standby recovery
//  1.05 : added LED test sysex command
//  1.06 : added: Reset after successful flashing, detach all USB before flashing, auto-detect board type
//  2.01 : use assigned ID's for USB and MIDI SysEx, shorter time-outs until stalling packets are dropped, fixed resume bug
//  2.02 : added Unique Device ID, auto-detect board type extended for latest PCB revision

>>> Firmware Update Instructions <<<

Note that the MIDI Bridge only accepts a firmware update when *no* MIDI traffic has occured since power-up, otherwise it will simply
try to deliver the MIDI data on the other port as in normal operation.

1. Fully disconnect the MIDI Bridge

2. Connect MIDI Bridge to PC only (which port used on the MIDI Bridge should not matter)

3a. For Linux Users, using "amidi" (https://www.systutorials.com/docs/linux/man/1-amidi/)
  - find hardware port ID with "amidi -l", say it was hw:0,0,0 for example
  - send SysEx with "amidi -p hw:0,0,0 -s nlmb-fw-update-VX.YZ.syx"  (X.YZ replaced with actual firmware number)

3b. For Windows/Mac users:
  - use an aplication like "MIDI Tools" (https://mountainutilities.eu/miditools)
  - load Firmware SysEx file
  - send to MIDI Bridge
  
If the firmware update was successful, the MIDI Bridge will show that by both LEDs blinking fast in bright green color
for 5 seconds, then it will reset itself and show the 
If not, try again the full cycle (note: try using also the other port of the MIDI bridge).

4. Fully disconnect the MIDI Bridge and reconnect. 

5. Optional Firmware Version Check:
- Software like "MIDI Tools" must be restarted and then will show the new firmware version in the setup screen.
- on Linux, use "usb-devices | grep -C 6 -i nonlinear"

6. Unique Device ID:
Sadly, Windows is not capable to show the Unique Device ID correctly which is part of the device name,
neither in the Device Manager nor anywhere else, including MIDI applications. 
This basically defeats the purpose of the Unique Device ID, telling several apart several attached MIDI Bridges.
Once Windows has detected a MIDI Bridge it will show any newly attached MIDI Bridges with the same name and ID
forever from now on.
The only way to refresh the display name is to remove all instances of the MIDI Bridge as seen by the Device Manager:
Go to device manager, select "show hidden devices", then delete all "NLL-Bridge" entries
(from the "Audio, Video and Gamecontroller" list), one by one. 
Do this while the MIDI Bridge is *not* plugged in, of course.

To our rescue there is an application called UsbTreeView (https://www.uwe-sieber.de/usbtreeview_e.html)
which shows the device name and unique ID properly.


----------------


>>> LED test  <<<
Files led-test(V2.x).syx (or led-test(V1.x).syx, for Firmware Versions 1.x)
If the first MIDI data sent to the Bridge is the LED-Test command sysex, then the LEDs will cycle through all colors
at full brightness. This is useful for testing and mechanical alignment during assembly.
The normal operation of the bridge is not affected, you just don't get the normal traffic display.
LED test mode can only be exited with a power cycle.


----------------


>>> Port Identification <<<

At power up, there is a blink pattern which indicates the firmware version.
The LED which flashes first, in yellow color, denotes the "High Speed" port, the other side is the "Full Speed" port.
