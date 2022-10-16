# sprdproto

Yet another thing that talks in the download mode protocol of Spreadtrum/Unisoc SoCs.

## sprdproto

`./sprdproto <payload 1 addr> <payload 1 file> [<payload 2 addr> <payload 2 file>]`

It tells all by itself.

The first payload is loaded on the BootROM stage, and so, the only thing you have is the internal SRAMs.

The second payload is loaded on the FDL1 stage, which is provided by the first payload, and in that,
you have access to the DRAM.

## sprdrunner.py

Not done yet... but for some insights:

`python3 sprdrunner.py [--dram] address file`

The `--dram` key is used to ensure that the DRAM is initialized (i.e. it will/should run FDL1).

The `address` and `file` is obviously the load address and the file that will be executed.
