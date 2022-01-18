# midibridge
 hardware + firmware + tools for a USB host-to-host MIDI bridge
 
## Project Organization
* firmware/src contains two projects:
  * the microcontroller main firmware application. Required packages: _cmake_, native _gcc_ and _gcc-arm-none-eabi_ cross-compiler, _libasound2-dev_. No docker encapsulation, etc. There also a project files for Windows-based LPCxpresso (for LPCXpresso v8.2.2_650 or newer) IDE (needed for flashing the initial firmware).
  * a uC firmware component 'in-app-flasher' to flash this firmware into the uC. The image of this flasher is uploaded into RAM via USB in form of a MIDI SysEx message and then executed. The image of the main firmware is contained (statically linked) within the in-app-flasher and hence the executed code can flash the new firmware. The final update image in form of a MIDI SysEx file can be found in the top build dir under `firmware/src/in-app-flasher/in-app-flasher.syx` and as a named duplictate `firmware/src/in-app-flasher/nlmb-fw-update-Va.bb.syx` with `a` being the major version number and `bb` being the two digit minor version number.
* tools/mk-sysex is a helper tool mainly for use at build-time to create the proper SysEx message from the binary image of the in-app-flasher.
* tools/perf-test is a helper tool to check/test the midi-bridge for general operation, data integrity and latency.
Those helper tools are compiled for the platform the _build system_ is running on (and again, no docker encapsulation).
* CMake switches, effective for the 'firmware' section (Usage is `cmake -DEVAL_BOARD=On|Off  -DSEPARATE_USB_DEVICE_IDS=On|Off  -DLONG_PACKET_TIMEOUTS=On|Off  -DBETA_FIRMWARE=On|Off  path/to/source-dir`) : 
  * `EVAL_BOARD` --> Force GPIOs for LEDs etc as wired on the evalution board. When set to 'Off' auto-detection will be used.
  * `SEPARATE_USB_DEVICE_IDS` --> use different USB device IDs and strings for HS and FS port. For debug/test. Note that this disables the Unique Device ID feature.
  * `LONG_PACKET_TIMEOUTS` --> Use long packet timeouts of 1s for the inital packet and 100ms for followling packets.For debug/test.
  * `BETA_FIRMWARE` --> Mark a firmware optically as Beta in the firmware version display by adding 3 times blinking red on both LEDs after version display. For debug/test.    

Toolchain setups are provided for two platforms:
* automated build of all components with CMake
* manual build of the main application with LPCXpresso under Windows. This is currently required to build the artifact needed for flashing the *blank* uC chip at production time with a hardware JTAG programmer. A proper LPXxpresso project is needed, both to create the \*.axf file first and then to flash the artifact anytime later.


### Inner dependencies
Whenever there is a change in any of the header parameters of the SysEx message (see doc/SysEx_DeviceInterface_V1.0.odt) in a new firmware revision, it is important to use the mk-sysex tool from the *current* firmware revision installed in the product to create the SysEx. Otherwise the device will reject the SysEx message.
Currently, the way to accomplish this in the build is to trick the build into using another, the older, *mk-sysex* than the one it just compliled:
- make a clean build (``make clean; make``) of the current firmware of the product and save the *mk-sysex* executable somewhere.
- switch to the new firmware and make a clean build.
- copy the saved *mk-sysex* into the build output dir replacing the new one, and run ``touch tools/mk-sysex/mk-sysex)`` to update its time-stamp.
- run *make* again, which now will detect that the *mk-sysex* executable has changed and thus will re-invoke the creation of the SysEx file for the update with the correct version of *mk_sysex*
