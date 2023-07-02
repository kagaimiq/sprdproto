# sprdproto / sprdrunner

Yet another thing that talks in the download mode protocol of Spreadtrum/Unisoc SoCs.

It loads a binary into the memory and runs it, either through the BootROM directly (without DRAM) or through FDL1 (with DRAM).

If you wanna flash, then use something else, or wait until this becomes yet another speadtrum flasher too.

Currently only the USB inteface is supported, support for UART might be added as well, at least it's available as a kind of PoC or something like that.

## sprdproto (sprdrunner)

Usage: `./sprdproto <payload 1 addr> <payload 1 file> [<payload 2 addr> <payload 2 file>]`

It tells all by itself, it takes one or two files, which are loaded and executed in order.

The first payload is either FDL1 or anything else, which is executed in the Boot ROM, so no DRAM is available.

The second payload is executed in the FDL1 mode, which is obviously provided by the FDL1 binary loaded earlier,
so you usually have the access to the DRAM (or other kind of relatively huge memory) as well.

## sprd_uarts.py

This is the script that does roughly the same but via UART instead of USB.

Usage: `sprd_uarts.py --port <port> <payload address> <payload file>`

The baudrate can be specified with the `--baud` parameter, however it is usually 115200 baud so you don't have to change it.
The rest is self-explanatory.

## Interfaces

### USB

The USB interface is activated by pressing some key on the keypad, which varies from device to device,
or the volume keys (which are usually connected to the keypad scanning hardware).

Sometimes you can tie some pin to GND (e.g. the ID pin on a USB connector or something like that).

Then it will appear on USB as a `1782:4d00` device.

### UART

As for the UART interface, it is always activated prior to the boot process.

For example, let's look at this output from UART on normal boot:

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

These five 'U's at the beginning is actually the part of the UART handshake,
on which the host should send three 0x7E bytes, which will eventually enter the
same protocol that USB uses. (except that for the first packet you send two 0x7E's instead of just one)

So for that, you just power up your device and it should work.
