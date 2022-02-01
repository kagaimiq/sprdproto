# sprdproto
Tool to upload code binaries into Spreadtrum SoCs via the BootROM/FDL protocol

This tool is prooven to work on SC7731G.

## Usage
The usage is as follows: `sprdproto <payload addr> <payload file> [<2nd payload addr> <2nd payload file>]`

where the first payload is loaded using the BootROM protocol (528 byte max block size and CRC16 checksums),
the second payload is later loaded using the FDL1 protocol (2112 byte max block size and Spreadtrum's own special checksum).

The tool will wait indefinetly for the device to be plugged in,
so that you can run this tool and then put your device into the UBD mode (usually achieved by holding the VOL-/VOL+ key while powering on).

## Requirements
This tool uses the libusb-1.0 library to communicate with the spreadtrum's own special serial device class.
Apart of that there is no other specific requirement.
