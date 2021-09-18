
import gdm
import struct
import numpy as np

from io import StringIO
from gdm_num import gdm_num



class GDM220(gdm.GDM):

    labels = {
#        57: (5, 'edm-raw-measurement', None, None), # no access from this interface (0x11)
#        30: (5, 'ppm', None, None), # edm ppm (0x86)
#        20: (5, 'PrismC', None, None), # prism constant (0x87)
#        28: (5, 'sD', None, None), # settingout distance(0x8c)
#        29: (5, 'xx', None, None), ??? (0x8d)
#        31: (5, 'VertAngleAdj', (-20, 20), None), #vertical angle adjust
#        34: (5, 'HT', None, None), #height of the station (0x89)
#        35: (5, 'Ih', None, None), #instrument height (0x8a)
#        36: (5, 'SH', None, None), # signal height (0x8b)
        209: (5, 'edm-constant', None, None), # edm offset (0x96)
        180: (5, 'geodat-write', None, None), #write variable (xx yy yy yy yy yy - xx-var num, yy-bcd value)
        181: (5, 'geodat-read', None, None), #read variable (xx - var num, then issue read)
    }

    variables = {
#intmem (doesnt survive poweroff)
# 0x1c - bank1 R4 - P7 + flags
#each variable address is (x & 0x1f) * 5 ... 0x0b * 5 = @55 @0x37
        0x07: ("temporary-add/mul", None),
        0x08: ("temporary-add/mul", None),

        0x09: ("?", None),
        0x0a: ("?", None),

        0x0b: ("temporary1", None),
        0x0c: ("temporary2", None),
        0x0d: ("temporary3", None),
        0x0f: ("temporary-vertical-distance-or-cal-angle1", None),
        0x10: ("temporary-setout-difference-or-cal-angle2", None),
        0x11: ("slope-distance", None),
        0x12: ("angle", None),
        0x13: ("cos(angle)", None),
        0x14: ("sin(angle)", None),

#extram bank0
#4bit wide ram, each variable is 10spaces
# variable address in ram is  (x & 0x1f) * 10 
# variable bank is (x & 0x60) >> 5
# bit 0x80 tells its in external ram
# 0x85 => 5th variable in 0 bank address 50 .. 0x32
# 0xf1 => 0x11 variable in 3 bank address 0x55 .. 85

# addr 0-0x10 used as register save space for isr
# addr 0x11-0x30 ?
# addr 0x31 - P6 state

        0x85: ("?", 0),
        0x86: ("ppm", 0),
        0x87: ("prism-constant", 0),
        0x88: ("vertical-angle-adj", 0),
        0x89: ("height-of-station", 0), # cleared on start
        0x8a: ("instrument-height", 0), # cleared on start
        0x8b: ("signal-height", 0),     # cleared on start
        0x8c: ("setout-distance", 0),   # cleared on start
        0x8d: ("vertical-distance-offset", 0), # cleared on start
        0x8e: ("?", 0),
        0x8f: ("horizontal-distance", 0),
        0x90: ("vertical-angle-offset", 0), #factory set?
        0x91: ("?", 0),
        0x92: ("working-copy-of-95", 0),
        0x93: ("angle-xxx", 0),
        0x94: ("ADC-reading-1", 0),
        0x95: ("ADC-reading-2", 0),
        0x96: ("edm-constant", 0),     # edm distance offset
        0x97: ("angle-const-inv-g", 0),
        0x98: ("angle-sensor-out",0 ), # value right before arcsin function

#constants start here
#extram bank1
        0xa0: ("not-used", 0),
        0xa1: ("angle-mul-1", 0),
        0xa2: ("angle-mul-2", 0),
        0xa3: ("angle-mul-3", 0),
        0xa4: ("angle-mul-4", 0),
        0xa5: ("angle-c1", 0),
        0xa6: ("angle-c1", 0),
        0xa7: ("angle-c1", 0),
        0xa8: ("angle-c2", 0),
        0xa9: ("angle-c2", 0),
        0xaa: ("angle-c2", 0),
        0xab: ("not-used", 0),
        0xac: ("poly-arcsin4", 0),
        0xad: ("poly-arcsin3", 0),
        0xae: ("poly-arcsin2", 0),
        0xaf: ("poly-arcsin1", 0),
        0xb0: ("poly", 0),
        0xb1: ("?", 0),
        0xb2: ("?", 0),
        0xb3: ("?", 0),
        0xb4: ("?", 0),
        0xb5: ("?", 0),
        0xb6: ("?", 0),
        0xb7: ("?", 0),
        0xb8: ("not-used", 0),

#extram bank2
        0xc0: ("ppm-set-limit-high", 0),    #+195
        0xc1: ("ppm-set-limit-low", 0),     # -60
        0xc2: ("prismc-set-step", 0),       #   1mm
        0xc3: ("prismc-set-limit-high", 0), #  10mm
        0xc4: ("prismc-set-limit-low", 0),  # -10mm
        0xc5: ("vertangadj-set-step", 0),   #     2mgons
        0xc6: ("vertangadj-set-limit-low", 0), # -2000
        0xc7: ("vertangadj-set-limit-high", 0),#  2000
        0xc8: ("angle_out_of_range", 50),   #50 gons or 45deg
        0xc9: ("?", 0),
        0xca: ("bankswitchrange1", 0),
        0xcb: ("bankswitchrange2", 0),
        0xcc: ("bankswitchrange3", 0),
        0xcd: ("bankswitchrange4", 0),
        0xce: ("?", 0),
        0xcf: ("?", 0),
        0xd0: ("?", 0),
        0xd1: ("?", 0),
        0xd2: ("num_1", 1),
        0xd3: ("num_0", 0),
        0xd4: ("?", 0),
        0xd5: ("?", 0),
        0xd6: ("?", 0),
        0xd7: ("?", 0),
        0xd8: ("?", 0),

#extram bank3
        0xe0: (" ", 0),
        0xe1: ("poly-sin4", 0), # ... 0?
        0xe2: ("poly-sin3", 0), # 0.172416
        0xe3: ("poly-sin2", 0), # 0.224706
        0xe4: ("poly-sin1", 0), # 3.118150
        0xe5: ("poly-cos3", 0),
        0xe6: ("poly-cos2", 0),
        0xe7: ("poly-cos1", 0),
        0xe8: ("3coef-1.0", 0),
        0xe9: ("3coef-1.1", 0),
        0xea: ("3coef-1.2", 0),
        0xeb: ("3coef-2.0", 0),
        0xec: ("3coef-2.1", 0),
        0xed: ("3coef-2.2", 0),
        0xee: ("3coef-3.0", 0),
        0xef: ("3coef-3.1", 0),
        0xf0: ("3coef-3.2", 0),
        0xf1: ("3coef-4.0", 0),
        0xf2: ("3coef-4.1", 0),
        0xf3: ("3coef-4.2", 0),
        0xf4: ("angle-some-coef1", 0),
        0xf5: ("angle-some-coef2", 0),
        0xf6: ("angle-some-coef3", 0),
        0xf7: ("angle-some-coef4", 0),
        0xf8: ("angle-some-coef5", 0),
        0xf9: ("checksum", 0), # only few bytes are valid
    }

    def __init__(self, gdmif, verbose=True):
        super().__init__(gdmif, verbose)

        for l,v in self.labels.items():
            if not len(v) == 2:
                raise Exception("fix variabl table", l)

    def write_label(self, label, value):
        addr = label & 0x1f
        if label & 0x80 == 0x80 and addr > 0x18:
            raise Exception("no such variable")

        val = gdm.to_bcd(value, 5) 
        if len(val) > 5:
            raise Exception("value")

        self.gdmif.send(180, struct.pack("B", label) + val)

    def read_label(self, label):
        addr = label & 0x1f
        if label & 0x80 == 0x80 and addr > 0x19:
            raise Exception("no such variable")

        self.gdmif.send(181, struct.pack("B", label))
        ret, in_lab, data = self.gdmif.recv()

        if not in_lab == 250:
            raise Exception("wrong in label")

        raw = from_bcd(data)

        print("BCD:", data, "decoded:", raw)

        return raw


    def load_defaults(self):
        for var, d in self.variables.items():
            if d[1] is not None:
                print(var, d[0], d[1])
                self.write_label(var, d[1])
                #time.sleep(0.1)

    def backup_labels(self, start, stop):
        do = OrderedDict()
        for var, d in self.labels.items():
            if var < start or var >= stop:
                continue 

            l = self.read_label(var)
            do[var] = l
            print(var, d[0], l)

        return do

