#A CHIP-8 emulator written in C
--------------------------------
The `CHIP-8` isn't an actual computer, but a language interpreter used on computers from the '70s and '80s, primarily the RCA TELMAC 1800 and the COSMAC VIP, both based on the CDP-1802 CPU. Hobbyist computers saw fairly large use, too. It was mostly used for making programming video games easier. Later, in the '90s, it saw widespread use on calculators as a programming tool for, again, developing video games on them.

For all intents and purposes, however, it's a complete system. A "virtual computer" if you will, and like any other computer can be emulated, given opcodes.

This is an emulator for the CHIP-8, written in C (possibly using Mark Kilgard's windowing system independent openGL library, GLUT, for graphics/input/sound)

Right now this project is nothing more than a debugger with a hardcoded ROM.
