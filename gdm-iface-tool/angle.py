# mode:
# 1 = send BA whatever
# 2 = compensator only
# 4 = raw data

import struct

LABEL_BUILD_CODE = 100
LABEL_VERSION_CODE = 179
LABEL_MEAN_VALUE = 110
LABEL_MEAN_VALUE_DELTA = 111
LABEL_OKTANT_H = 113
LABEL_OKTANT_V = 114
LABEL_AD_COS_H = 115
LABEL_AD_SIN_H = 116
LABEL_AD_COS_V = 117
LABEL_AD_SIN_V = 118
LABEL_AD_OFFSET = 119
LABEL_AD_COMP_REF = 120
LABEL_AD_COMP_X = 122
LABEL_AD_COMP_Y = 123
LABEL_DATA_8A = 138
LABEL_VA_COEF_TABLE = 145
LABEL_VA_COEF_COUNT = 146
LABEL_HA_COEF_TABLE = 147
LABEL_HA_COEF_COUNT = 148
LABEL_MODE = 157
LABEL_DATA_A7 = 167
LABEL_ANGLE_H = 168
LABEL_ANGLE_V = 169
LABEL_DATA_BA = 186

angle0_file = "4400-a1.bin"
angle1_file = "4400-a1.bin"

angle_coef_1 = 88
angle_coef_2 = 12

labels = {
    186: ("DATA_BA", 1),
    168: ("ANGLE_H", 4),
    113: ("OKTANT_H", 1),
    114: ("OKTANT_V", 1),
    115: ("COS_H", 4),
    116: ("SIN_H", 4),
    117: ("COS_V", 4),
    118: ("SIN_V", 4),
    119: ("OFFSET", 4),
    120: ("COMP_REF", 2),
    122: ("COMP_X", 2),
    123: ("COMP_Y", 2),
    168: ("ANGLE_H", 4),
    169: ("ANGLE_V", 4),
    0xff: ("xy", 2),
}

class Angle4000:
    def __init__(self, gdmif):
        self.gdmif = gdmif

        self.gdmif.register_pkt_handler(self.packet_handler)

    def write(self, angle_label, data):
        self.gdmif.send(angle_label, data)

    def read(self, angle_label):
        self.gdmif.send(253, struct.pack("B", angle_label))
        ret, in_lab, data = self.gdmif.recv()
        return data

    def initialize(self):

        with open(angle0_file, 'rb') as fin0, open(angle1_file, 'rb') as fin1:
            angle0_bin = fin0.read()
            angle1_bin = fin1.read()

        expbin0 = angle_coef_1 + angle_coef_2 * 6 
        expbin1 = angle_coef_1 + angle_coef_2 * 6 

        if len(angle0_bin) != expbin0 or len(angle1_bin) != expbin1:
            raise Exception("coef len needs to be", expbin0, expbin1, " got:", len(angle0_bin), len(angle1_bin))

        self.write(LABEL_VA_COEF_TABLE, angle0_bin)
        self.write(LABEL_HA_COEF_TABLE, angle1_bin)

        self.write(LABEL_VA_COEF_COUNT, struct.pack("<B", 6))
        self.write(LABEL_HA_COEF_COUNT, struct.pack("<B", 6))

        self.write(LABEL_MEAN_VALUE, struct.pack("<B", 165))
        self.write(LABEL_MEAN_VALUE_DELTA, struct.pack("<I", 2000))

        self.write(LABEL_MODE, struct.pack("B", 0))

    def get_build_code(self):
        code = self.read(LABEL_BUILD_CODE)
        code = bytearray(code)
        code.reverse()
        return code[:-1].decode('ascii')

    def get_version_code(self):
        code = self.read(LABEL_VERSION_CODE)
        code = bytearray(code)
        code.reverse()
        return code[:-1].decode('ascii')

    def set_mode(self, mode):
        self.write(LABEL_MODE, struct.pack("B", mode))

    def packet_handler(self, pkt):
        print("New packet", pkt)
        
        while True:
            if pkt[0] not in labels:
                raise Exception("no such label %d" % pkt[0])

            lbl = pkt[0]
            length = pkt[1]

            name =  labels[lbl]

            if len(pkt) < 2+length:
                print("malformed packet")
                break

#            print(length, pkt)
            if length == 1:
                data = pkt[2]
            elif length == 2:
                data, = struct.unpack(">H", pkt[2:2+length])
            elif length == 4:
                data, = struct.unpack(">I", pkt[2:2+length])
            else:
                raise Exception("unknown length")

            print(f"{name} {data:x}")

            pkt = pkt[2+length:]
            if len(pkt) == 0:
                break


if __name__ == "__main__":
    from gdmif import GDMIf, GDMIfSub
    import time

    gdmif = GDMIf("/dev/ttyACM0")
    gdmif.start()

    print("ID '%s'" % (gdmif.get_id()))

    gdmif1 = GDMIfSub(gdmif, 1)
    gdmif1.set_mode(0)
    gdmif1.set_timing(5000, 5, 10)

    angle = Angle4000(gdmif1)

    print(angle.read(LABEL_MEAN_VALUE_DELTA))
    angle.initialize()
    print(angle.get_build_code())
    print(angle.get_version_code())

    angle.set_mode(4)
    gdmif1.set_mode(1)

    #angle.gdmif.send(LABEL_MODE, struct.pack("B", 2))

    while True:
        try:
            time.sleep(1)
            angle.set_mode(4)
        except KeyboardInterrupt:
            break

    angle.set_mode(0)
    time.sleep(1)
    gdmif1.set_mode(0)

    gdmif.stop()
