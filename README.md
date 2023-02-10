# DMK (deemonoise's MIDI kontroller)

4x4 MIDI controller with arcade buttons.
It has two working modes:
Note - regular mode. You can select scale, root note and transpose by octaves. You can also turn on hold (great for arpegios)
Blackbox - mappings for 101music blackbox PAD and SEQ.

It uses two arduino nanos connected by serial software. The 1st nano is used as a keyboard controller and sends the keys to the 2nd one. 2nd receives keys and sends MIDI commands. This is for 16-note polyphony. 

## BOM

|  |  |
|--|--|
| Arduino nano |2  |
|Arcade button 24mm|16|
|OLED 0.96'' i2c|1|
|Rotary encoder|1|
|MIDI connector|1|