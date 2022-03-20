
import gdm
import struct
import numpy as np

from io import StringIO
from gdm_num import gdm_num
#todo:
# make some nice structure for angles



class GDM400(gdm.GDM):
    labels = {
        0: (5, '  Info', None, '0'),
        1: (5, '  Data', None, '0'),
        2: (5, '   Stn', None, '0'),
        3: (1, '    Ih', None, 0.0),
        4: (5, ' Pcode', None, '0'),
        5: (5, '   Pno', None, '0'),
        6: (1, '    SH', None, 0.0),
        7: (1, '    HA', None, None),
        8: (1, '    VA', None, None),
        9: (1, '    SD', None, None),
        10: (1, '   DHT', None, 0.0),
        11: (1, '    HD', None, 0.0),
        15: (5, '  Area', None, '0'),
        16: (1, '    dH', None, 0.0),
        17: (1, '  HA  ', None, 0.0),
        18: (1, '  VA  ', None, 0.0),
        19: (1, '    dV', None, 0.0),
        20: (1, 'Offset', (-1.0, 1.0), 0.0),
        21: (1, 'HA ref', None, 0.0),
        22: (7, '  Comp', None, None),
        24: (1, '  HA I', None, 0.0),
        25: (1, '  VA I', None, 0.0),
        27: (1, '   SHA', None, 0.0),
        28: (1, '   SHD', None, 0.0),
        29: (1, '   SHT', None, 0.0),
        30: (1, '   PPM', (-60.0, 195), None),
        35: (1, '     S', None, 0.0),
        36: (1, 'Ht.Ofs', None, 0.0),
        37: (1, '     N', None, 0.0),
        38: (1, '     E', None, 0.0),
        39: (1, '   ELE', None, 0.0),
        40: (1, '    dN', None, 0.0),
        41: (1, '    dE', None, 0.0),
        42: (1, '  dELE', None, 0.0),
        43: (1, 'Utm_Sc', None, 0.0),
        44: (1, ' Slope', None, 0.0),
        45: (1, '   dHA', None, 0.0),
        46: (1, ' S_dev', None, 0.0),
        47: (1, '    Nr', None, 0.0),
        48: (1, '    Er', None, 0.0),
        49: (1, '    VD', None, 0.0),
        50: (5, 'Job No', None, '0'),
        51: (5, '  Date', None, None),
        52: (5, '  Time', None, None),
        53: (5, 'Operat', None, '0'),
        54: (5, '  Proj', None, '0'),
        55: (2, 'Instno', None, None),
        56: (1, '  Temp', None, 20),
        58: (1, 'Ea rad', None, 0.0),
        59: (1, 'Refrac', None, 0.0),
        60: (5, 'ShotID', None, '0'),
        61: (5, 'Active', None, '0'),
        62: (5, 'Refobj', None, '0'),
        63: (1, '  Diam', None, 0.0),
        64: (1, 'Radius', None, 0.0),
        65: (1, 'Geomcode', None, 0.0),
        66: (1, 'Figcode', None, 0.0),
        67: (1, '   SON', None, 0.0),
        68: (1, '   SOE', None, 0.0),
        69: (1, '   SHT', None, 0.0),
        70: (5, 'ObjID', None, '0'),
        71: (5, 'ObjNo', None, '0'),
        72: (1, 'Radofs', None, 0.0),
        73: (1, 'RT.ofs', None, 0.0),
        74: (1, ' Press', None, 1000),
        75: (1, '   dHT', None, 0.0),
        76: (1, '   dHD', None, 0.0),
        77: (1, '   dHA', None, 0.0),
        78: (5, '   COM', None, '1.8.0.9600'),
        79: (6, '   End', None, ord('>')),
        80: (1, ' Sect.', None, 0.0),
        81: (1, 'A-prm.', None, 0.0),
        82: (1, 'SecInc', None, 0.0),
        83: (1, 'CL.ofs', None, 0.0),
        84: (5, 'PCoeff', None, '0'),
        85: (5, '   PHt', None, '0'),
        86: (5, ' Layer', None, '0'),
        87: (5, 'LayerH', None, '0'),
        88: (5, 'Profil', None, '0'),
        89: (5, ' Dist.', None, '0'),
        90: (5, '  User', None, '0'),
        91: (5, '  User', None, '0'),
        92: (5, '  User', None, '0'),
        93: (5, '  User', None, '0'),
        94: (5, '  User', None, '0'),
        95: (5, '  User', None, '0'),
        96: (5, '  User', None, '0'),
        97: (5, '  User', None, '0'),
        98: (5, '  User', None, '0'),
        99: (5, '  User', None, '0'),
        100: (5, 'sw-build', None, None),
        101: (5, 'sw-ver', None, None),
        102: (1, 'ang_preset_ha', None, 0.0),
        103: (1, 'ang_offset_va', None, 0.0),
        104: (1, 'comp1_gain', None, 1.0),
        105: (5, 'bat_date', None, None),
        106: (1, 'comp2_gain', None, 1.0),
        107: (1, 'comp1_off', None, 0.0),
        108: (1, 'comp2_off', None, 0.0),
        109: (1, 'comp_cross', None, 0.0),
        110: (6, 'sensor_mode', None, 0),
        111: (4, 'sensor_xxx', None, None),
        112: (6, '', None, None),
        113: (4, 'ang_sect_h', None, None),
        114: (4, 'ang_sect_v', None, None),
        115: (4, 'ang_val_cos_h', None, None),
        116: (4, 'ang_val_sin_h', None, None),
        117: (4, 'ang_val_cos_v', None, None),
        118: (4, 'ang_val_sin_v', None, None),
        119: (4, 'ang_ref', None, None),
        120: (4, 'comp_ref', None, None),
        121: (1, 'operating_time', None, 0.0),
        122: (4, 'comp_val_x', None, None),
        123: (4, 'comp_val_y', None, None),
        124: (6, '', None, None),
        125: (4, '', None, None),
        126: (4, '', None, None),
        127: (4, '', None, None),
        128: (4, '', None, None),
        129: (4, '', None, None),
        130: (2, 'sens_ha_no', None, None),
        131: (2, 'sens_va_no', None, None),
        132: (4, '', None, None),
        133: (4, '', None, None),
        134: (6, '', None, None),
        136: (4, 'chksum', None, None),
        137: (6, '', None, None),
        138: (1, 'plumb_noice', None, 0.0),
        139: (1, 'ang_corr_va', None, None),
        140: (1, 'ang_corr_ha', None, None),
        141: (6, '', None, None),
        142: (2, 'label_ptr', None, None),
        143: (2, '', None, None),
        144: (2, 'texts_ptr', None, None),
        145: (2, 'ang1_corr_ptr', None, None),
        146: (6, 'ang1_corr_cnt', None, None),
        147: (2, 'ang2_corr_ptr', None, None),
        148: (6, 'ang2_corr_cnt', None, None),
        149: (6, 'fkey-digits', None, None),
        150: (1, 'inst_center_corr_v', None, 0.0),
        151: (1, 'inst_center_corr_h', None, 0.0),
        152: (2, 'comp_no', None, None),
        153: (5, 'config', None, None),
        154: (1, 'comp1_out_h', None, None),
        155: (5, 'model', None, None),
        156: (5, 'owner', None, None),
        157: (2, 'distance_meter_no', None, None),
        158: (4, '', None, None),
        159: (4, '', None, None),
        160: (4, 'comm-flags', None, None),
        161: (4, '', None, None),
        162: (4, '', None, None),
        163: (4, 'proc-flags', None, None),
        164: (4, '', None, None),
        165: (1, 'ang0_out-ha', None, None),
        166: (1, 'ang1_out-va', None, None),
        167: (1, 'comp_avg_rounds', None, 100.0),
        168: (1, 'comp1_acc', None, None),
        169: (1, 'comp2_acc', None, None),
        170: (2, 'unknown', None, None),
        171: (2, 'edm_coef_ptr', None, None),
        172: (4, 'edm_diode_type', None, None),
        173: (2, 'some_ptr', None, None),
        174: (2, 'some_ptr', None, None),
        183: (1, 'horiz-offset', (-0.05, 0.05), 0.0),
        209: (1, 'edm_constant', None, 0.0),
        210: (1, 'Coll-error-ha', None, 0.0),
        211: (1, 'Coll-error-va', None, 0.0),
        212: (1, 'Tilt-ax-error', None, 0.0),
        213: (3, 'InternalInstNo', None, None),
        251: (7, 'comm', None, None),
        252: (1, 'comm', None, None),
        253: (6, 'comm', None, None),
        254: (4, 'comm', None, None),
    }

    angle_coef_size = 240
    label_name_size = 600
    texts_size = 600
    
    def __init__(self, gdmif, verbose=True):
        super().__init__(gdmif, verbose)

    def load_label_names(self, label_names):
        if not len(label_names) == self.label_name_size:
            raise Exception("label names need " + str(self.label_name_size) + " bytes")

        addr = self.read_label(142)
        self.write_mem(addr, 0x0000, label_names)

    def backup_label_names(self):
        addr = self.read_label(142)
        ln = self.read_mem(addr, 0x0000, self.label_name_size)
        return (ln, )

    def load_texts(self, texts):
        if not len(texts) == 2:
            raise Exception("texts needs 2")

        addr = self.read_label(144)
        test_addresses = self.read_mem(addr, 0x0000, 6*2)
        addr1, seg1, addr2, seg2, size1, size2 = struct.unpack("HHHHHH", test_addresses)

        self.write_mem(addr1, seg1, texts[0])
        self.write_mem(addr2, seg2, texts[1])

        sizes = struct.pack("<HH", len(texts[0]), len(texts[1]))
        self.write_mem(addr+8, 0x0000, sizes)

    def backup_texts(self):
        addr = self.read_label(144)
        test_addresses = self.read_mem(addr, 0x0000, 6*2)
        addr1, seg1, addr2, seg2, size1, size2 = struct.unpack("HHHHHH", test_addresses)

        t1 = self.read_mem(addr1, seg1, size1)

        if (size2 == 0):
            return (t1, )

        t2 = self.read_mem(addr2, seg2, size2)

        return t1, t2

    def load_edm_coefs(self, coef_type, edm_bin=None):
        addredm = self.read_label(171)
        if coef_type == 0:
            b = b'\x00\x00' 
            # we write only 2 bytes, that mean type of calibration
            self.write_mem(addredm, 0x0000, b) # label 172
            #self.write_label(172, 0)
        else:
            b = struct.pack("<H", coef_type) + edm_bin
            # we upload whole array
            self.write_mem(addredm, 0x0000, b)

    def backup_edm_coefs(self):
        addredm = self.read_label(171)
        edmdata = self.read_mem(addredm, 0x0000, 2 + 4*6 + 4*3*6)
        return (edmdata,)


    def load_angle_coefs_text(self, file1, file2):

        b = []

        for fil in [file1, file2]:
            cal = []

            with open(fil) as f:
                while True:
                    line = f.readline()

                    if len(line) == 0:
                        break

                    elif 'cx' in line:
                        line = f.readline()
                        sio = StringIO(line)
                    elif 'cy' in line:
                        line = f.readline()
                        sio = StringIO(line)
                    elif 'coef' in line:
                        rest = f.read()
                        sio = StringIO(rest)
                    else:
                        raise Exception("wrong format")

                    coef = np.loadtxt(sio, delimiter=',')
                    cal.append(coef)

            a=bytearray()

            for x in cal[0]:
#                print(x)
                a += gdm_num.to_6num(x)

            for y in cal[1]:
#                print(y)
                a += gdm_num.to_6num(y)

            for row in cal[2]:
#                print(1/row[0])
                a+= gdm_num.to_6num(1/row[0])
#                print(row[1])
                a+= gdm_num.to_6num(row[1])
#                print(row[2])
                a+= gdm_num.to_6num(row[2])
            b.append(a)

        self.load_angle_coefs(b[0], b[1])
        
    def load_angle_coefs(self, angle0_bin, angle1_bin, angle0_count=6, angle1_count=6):
        if len(angle0_bin) != self.angle_coef_size or len(angle1_bin) != self.angle_coef_size:
            raise Exception("coef len needs to be", self.angle_coef_size, " got:", len(angle0_bin), len(angle1_bin))

        s = 0
        for i in range(int(len(angle0_bin)/2)):
            x, = struct.unpack("<H", angle0_bin[2*i:2*i+2])
            s += x
            s = s & 0xffff

        for i in range(int(len(angle1_bin)/2)):
            x, = struct.unpack("<H", angle1_bin[2*i:2*i+2])
            s += x
            s = s & 0xffff

        s = 0x10000 - s
        s = s & 0xffff

        addr0 = self.read_label(145)
        self.write_mem(addr0, 0x0000, angle0_bin)
        self.write_label(146, angle0_count)
        addr1 = self.read_label(147)
        self.write_mem(addr1, 0x0000, angle1_bin)
        self.write_label(148, angle1_count)
        self.write_label(136, s)

    def backup_angle_coefs(self):
        addr0 = self.read_label(145)
        a0 = self.read_mem(addr0, 0x0000, self.angle_coef_size)
        addr1 = self.read_label(147)
        a1 = self.read_mem(addr1, 0x0000, self.angle_coef_size)
        return a0, a1


    # valid: 100, 101, 200, 300, 400, 500, 600, 601, 700
    # 101 - all programs?
    def load_license(self, station_type):

        if isinstance(station_type, int):
            b = "%d" % station_type
        elif not isinstance(station_type, str):
            raise Exception("lic needs string")

        self.write_label(153, b)

