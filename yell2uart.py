import serial

def crc16(data):
    crc = 0
    for b in data:
        crc ^= b << 8
        for i in range(8):
            crc = ((crc << 1) ^ (0x1021 if crc >> 15 else 0)) & 0xffff
    return crc

def hdlc_pack(data):
    odata = b''

    data += crc16(data).to_bytes(2, 'big')

    for b in data:
        if b in (0x7d, 0x7e):
            odata += b'\x7d'
            b ^= 0x20

        odata += bytes([b])

    return b'\x7e' + odata + b'\x7e'


with serial.Serial('/dev/ttyUSB0', 115200, timeout=1) as port:
    port.write(b'\x7e\x7e\x7e\x7e\x7e\x7e\x7e\x7e\x7e\x7e\x7e')
    print(port.read(128))
