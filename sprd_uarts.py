import argparse, struct
import serial, crcmod

ap = argparse.ArgumentParser(description='Spreadtrum UART special')

def anyint(s):
    return int(s, 0)

ap.add_argument('--port', required=True,
                help='Serial port where the Spreadtrum device is connected to')

ap.add_argument('--baud', type=int, default=115200,
                help='Baudrate to use (default is %(default)d baud)')

ap.add_argument('addr', type=anyint,
                help='Load address')

ap.add_argument('file',
                help='File to load')

args = ap.parse_args()

######################################################################

calc_crc16 = crcmod.mkCrcFun(0x11021, initCrc=0, rev=False)

def calc_sprdchk(data):
    chk = 0

    while len(data) > 0:
        chk += int.from_bytes(data[:2], 'big')
        data = data[2:]

    chk = ((chk >> 16) + (chk & 0xffff)) & 0xffff
    chk = ~(chk + (chk >> 16)) & 0xffff
    return chk

######################################################################

with serial.Serial(args.port, args.baud, timeout=1) as port:
    print("Waiting for sane activity on the port..")

    port.reset_input_buffer()
    cnt = 0

    while True:
        rd = port.read(1)
        if rd == b'': continue

        if rd == b'\x55':
            cnt += 1
        else:
            cnt = 0

        if cnt == 4:
            port.write(b'\x7e' * 3)
            break

    print("Got it!")

    #-------------------------------------------------------------#

    def send_raw_packet(data):
        odata = b''

        for b in data:
            if b in (0x7d, 0x7e):
                odata += b'\x7f'
                b ^= 0x20
            odata += bytes([b])

        port.write(b'\x7e' + odata + b'\x7e')

    def recv_raw_packet():
        data = b''
        started = False

        while True:
            rx = port.read(1)
            if rx == b'': break

            if not started:
                if rx == b'\x7e':
                    started = True
            else:
                if rx == b'\x7e':
                    break

                elif rx == b'\x7d':
                    rx = port.read(1)
                    if rx == b'': break

                    rx = bytes([rx[0] ^ 0x20])

                data += rx

        return data

    def send_packet(cmd, data):
        pdata = struct.pack('>HH', cmd, len(data)) + data
        pdata += struct.pack('>H', calc_crc16(pdata))
        print('>>', pdata.hex())
        send_raw_packet(pdata)

    def recv_packet():
        pdata = recv_raw_packet()

        if len(pdata) < (2+2+2):
            raise Exception('Received packet is too short')

        resp, dlen = struct.unpack('>HH', pdata[:4])
        if len(pdata) < (2+2+dlen+2):
            raise Exception("Received packet's length is less than it claims")

        check = int.from_bytes(pdata[4+dlen:][:2], 'big')
        pdata = pdata[:4+dlen]

        if check != calc_crc16(pdata):
            raise Exception('Checksum mismatch')

        return resp, pdata[4:]

    #-------------------------------------------------------------#

    BSL_CMD_CONNECT             = 0x0000
    BSL_CMD_START_DATA          = 0x0001
    BSL_CMD_MIDST_DATA          = 0x0002
    BSL_CMD_END_DATA            = 0x0003
    BSL_CMD_EXEC_DATA           = 0x0004

    BSL_REP_ACK                 = 0x0080
    BSL_REP_VER                 = 0x0081

    send_raw_packet(b'')
    print(recv_packet())

    send_packet(BSL_CMD_CONNECT, b'')
    print(recv_packet())

    with open(args.file, 'rb') as f:
        size = f.seek(0, 2)
        f.seek(0)

        send_packet(BSL_CMD_START_DATA, struct.pack('>II', args.addr, size))
        print(recv_packet())

        while True:
            data = f.read(512)
            if data == b'': break

            send_packet(BSL_CMD_MIDST_DATA, data)
            print(recv_packet())

        send_packet(BSL_CMD_END_DATA, b'')
        print(recv_packet())

        send_packet(BSL_CMD_EXEC_DATA, b'')
        print(recv_packet())
