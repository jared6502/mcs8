# mcs8

A simple emulator for MCS-8 based computers.

Currently partially emulates an Intellec-8/Mod 8 computer system. The main window accepts keyboard input, which is received via the emulated UART on port 0, and currently prints text to the console via the emulated UART on port 8. Flags are present to enable a couple non-standard extensions, specifically a 16-level call stack (which was available on the Datapoint 2200) and an extended 64k memory range.

This emulator has been tested to function using Intel's MON-8 V3.0 as listed in publication 98-022B. The emulator will load this ROM and boot from it. A copy of the ROM is not included but the document can easily be obtained in an internet search.

Planned additions (mostly I/O options):
-CRT display (selected based on chosen devices)
-front panel (selected based on the chosen system to emulate)
-paper tape reader/punch (read and write a file)
-256-level data stack as listed in The Digital Group's Packet #1
-display as listed in The Digital Group's Packet #1
-Mark-8 TVT board
-cassette tape drive
-Datapoint 2200-specific stuff
-ROM images that can be distributed with the emulator
