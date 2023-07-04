# sprdproto

_Yet another thing that talks in the download mode protocol of Spreadtrum/Unisoc SoCs._

Tool to upload and run code into Spreadtrum SoCs via the BootROM (without access to DRAM) or FDL1 (with access to DRAM) protocol.

This tool is prooven to work on SC7731G (aka SC8830). But it might work with other SoCs as well.

If you wanna flash, then either use something else or wait until it becomes yet another spreadtrum flash tool.

## Usage

### sprdproto (sprdrunner)

- `sprdproto <payload addr> <payload file> [<2nd payload addr> <2nd payload file>]`
- e.g. `sprdproto 0x50000000 ../Devices/Micromax_Q379/FDL1.bin 0x80000000 ../sprd_prog_1/payload-sc7731g-Jielitech/sprdpayload.bin`

The first payload is loaded via the BootROM protocol (528 byte max block size and CRC16 checksum),
and the second payload is loaded via the FDL1 protocol (2112 byte max block size and Spreadtrum's own special checksum)

The tool will wait indefinetly for the device to be plugged in, so that you can run this tool and then put your device into the UBD (USB Download) mode,
which is usually achieved by pressing some key on a keypad, pressing some volume key or shorting some pin on power up.

### sprd_uarts.py

- `sprd_uarts.py --port <port> <payload addr> <payload file>`
- e.g. `sprd_uarts.py --port /dev/ttyUSB0 0x50000000 ../sprd_prog_1/payload-sc7731g/sprdpayload.bin`

This is the same as in sprdrunner, but it is done via UART instead of USB.

This is merely a proof of concept, or something like that.
This functionality might be added into sprdrunner and other tools, or they might be completely rewritten in python, but we'll see.

You can also specify the baudrate with the `--baud` parameter, but usually it is 115200 baud so you don't have to change it.

This will wait until some sane activity will be going in the UART port you chose:

When the SoC just powers up, it sends a series of 'U's on the UART so that the host tool can then send out three 0x7E bytes
to signal an acknowledge, and then proceed to use the same protocol as in USB (i.e. packets encapsulated into HDLC-like packets)

So you can just start this tool and then turn on your device, and it should detect it.

This is an example of the UART output on normal(-ish) boot:

```
UUUUUMCI
ERS
ROM
HSS

ddr auto detect cs0 ok!
ddr auto detect cs1 fail!
 <<    KBoot for Spreadtrum SC7731G    >> 
build @ Jun  6 2022 20:13:23
Btn0=77
```

Here you can see that first goes five 'U's, on which it waits for three 0x7E bytes,
and then the `MCI`, `ERS`, `ROM` and `HSS` messages from the ROM's boot process.

