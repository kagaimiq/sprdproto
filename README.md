# sprdproto / sprdrunner

Yet another thing that talks in the download mode protocol of Spreadtrum/Unisoc SoCs.

Currently it only loads a binary into the memory and runs it, either through the BootROM directly (no DRAM) or through FDL1 (with DRAM).
-- and that's a primary focus anyway.

Only the USB interface is supported at the moment.

## sprdproto

Usage: `./sprdproto <payload 1 addr> <payload 1 file> [<payload 2 addr> <payload 2 file>]`

It tells all by itself.

The first payload is loaded on the BootROM stage, and so the only memory that you have is the internal SRAMs.

The second payload is loaded on the FDL1 stage that is provided by the first payload, and here you have access to the DRAM.

However working with FDL1 might be difficult because of all the insanity that happens with it.
For example, different FDL1s (for the same SoC but for different devices) allow different portions of DRAM to be written compared to another FDL1;
if you try to write something huge (like a Linux kernel) then it will either error out immediatly (e.g. data is too huge) or somewhere on the way (maybe it breaks something).
