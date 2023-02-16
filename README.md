# sprdproto

Yet another thing that talks in the download mode protocol of Spreadtrum/Unisoc SoCs.

## sprdproto

`./sprdproto <payload 1 addr> <payload 1 file> [<payload 2 addr> <payload 2 file>]`

It tells all by itself.

The first payload is loaded on the BootROM stage, and so, the only thing you have is the internal SRAMs.

The second payload is loaded on the FDL1 stage, which is provided by the first payload, and in that,
you have access to the DRAM.
