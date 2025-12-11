# SRM-controller
A project for Nissan Leaf park actuator SRM motor control and encoder feedback. Simple button, led and audio feedback interface. All HW and SW design by OH2NLT, project is published with permission.

![SRM-controller.jpg](https://github.com/Romukeisari/SRM-controller/blob/main/SRM-controller.jpg)

# Hardware has the following:
- Arduino nano is used as controller
- Three configurable control inputs, meaning they can be used switching high or switching low depending on your application
- Two inputs for hall encoder
- Two configurable outputs for relay / lamp / led
- Output for three phase SRM motor (the design is tested with Leaf park actuator motor)
- Power supply for motor power can be shared with the controller, or they can be separated if you desire
- Motor power supply protected by polyfuse

# Hardware files:
- All hardware files are offered AS IS.
- SRM_gerber.zip made with original design on Pads. PCB can be ordered with this
- SRM_KiCad_designfiles.zip contains KiCad design files. They were converted from old Pads design. Files were opened first with newer Pads VX and then exported and saved to Altium project, which was then imported to KiCad. So be aware that there may be some discrepancies of bugs of any sorts. On a quick overview things look like they should.

# Software V 1.0 with the following functionality:
- SRM motor control with encoder feedback
- Safety functionality, brake pedal must be pressed before park lock operates
- Serial communication for functionality and fault feedback
- Fault detection forward and reverse direction (over/under travel and stall)
- Audio feedback on succesful lock/unlock

# Some observations on Leaf park actuator
- Motor resistance 1,35ohm / phase
- Phase inductance wary between about 1,6mH to 4,4mH depending rotor position
- phase coil saturation. On pole ~4ms, between poles ~1ms

- Tests with Leaf transmission, stroke for lock/unlock movement is approx 104 encoder A or B pulses
- actuator movement about 34-35 deg
