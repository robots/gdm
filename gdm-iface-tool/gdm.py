
import struct
import time
import sys
from datetime import datetime
from dateutil.relativedelta import relativedelta
from collections import OrderedDict

def to_bcd(value, length=1, pad=b'\x00'):
    sign = 1
    if value < 0:
        sign = -1
        value = -value

    ret = bytearray(0)
    while value:
        value, ls4b = divmod(value, 10)
        value, ms4b = divmod(value, 10)
        v = (ms4b << 4) + ls4b
        #print(v)
        ret += v.to_bytes(1, byteorder='little')

    if sign == -1:
        ret += b'\xa0'

    return pad * (length - len(ret)) + ret

def from_bcd(in_bytes):
    ret = 0

    bb = bytearray(in_bytes)
    bb.reverse()

    sign = 1
    for b in bb:
        ret *= 100
        d = (b >> 4)
        if d > 9:
            sign = -1
            d = 0
        d1 = b & 0xf
        if d1>9:
#            print("tutu", d1)
            sign = -1
            d1=0
        ret += d * 10 + d1

    return ret * sign

class GDM:
    TYPE_6NUM = 1
    TYPE_INT = 2
    TYPE_WHAT_1 = 3
    TYPE_SHORT = 4
    TYPE_STRING = 5
    TYPE_BYTE = 6
    TYPE_WHAT_2 = 7

    labels = {}

    def __init__(self, gdmif, verbose=True):
        self.gdmif = gdmif
        self.verbose = verbose

        for l,v in self.labels.items():
            if not len(v) == 4:
                raise Exception("fix label table", l)


    def get_type(self, label):
        if label not in self.labels.keys():
            raise Exception("no such label")

        l = self.labels[label]

        return l[0]

    def write_label(self, label, data):
        t = self.get_type(label)

        if t == GDM.TYPE_STRING:
            if not isinstance(data, str):
                raise Exception("label %d needs string" % label)

            data = bytearray(data.encode('ascii'))
            data.reverse()

            self.gdmif.send(251, b'\x00')
            self.gdmif.send(label, data)

        elif t == GDM.TYPE_BYTE or t == GDM.TYPE_SHORT or t == GDM.TYPE_INT:
            if isinstance(data, str):
                data = int(data)

            if data > 255 and t == GDM.TYPE_BYTE:
                raise Exception("label %d - value %d wont fit in byte" % (label, data))
            elif data > 2**16 and t == GDM.TYPE_SHORT:
                raise Exception("label %d - value %d wont fit in word" % (label, data))
            elif data > 2**32 and t == GDM.TYPE_INT:
                raise Exception("label %d - value %d wont fit in dword" % (label, data))

            self.gdmif.send(label, b'\x00\x00' + to_bcd(data))
        elif t == GDM.TYPE_6NUM:
            if isinstance(data, (str, int, float)):
                data = float(data)
            else:
                raise Exception("bad data type")
            
            po = 4

            #print(data,p, po, data * (10**po))
            ldata = to_bcd(int(data * (10**po)))
            print("label", label, "data", data, "p:", po, "bcd:", ldata)
            self.gdmif.send(label, ldata)

        else:
            raise Exception("not implemented")

    def read_label(self, label):
        t = self.get_type(label)

        self.write_label(160, 32)

        self.gdmif.send(253, b'\x00\x00' + to_bcd(label))
        ret, in_lab, data = self.gdmif.recv()

        if not ret == 0:
            raise Exception("error reading")

    
        #print("ok, lab: %d data: %s" % (in_lab, repr(data)))

        if t == GDM.TYPE_STRING:
            if not in_lab == 251:
                raise Exception("string should come from 251 not %d" % in_lab)

            data = bytearray(data)
            data.reverse()
            parts = data.split(b'=')
            inlabel = int(parts[0])
            if not inlabel == label:
                raise Exception("read %d but response %d" % (label, inlabel))

            return parts[1].decode('ascii')
        elif t == GDM.TYPE_BYTE or t == GDM.TYPE_SHORT or t == GDM.TYPE_INT:
            if not in_lab == label:
                raise Exception("read %d but response %d" % (label, in_lab))

            # normally this would be from_bcd(xx) / 10000
            return from_bcd(data[2:])

        elif t == GDM.TYPE_6NUM:
            # read label parameter first
            addr = self.read_label(143)
#            print("addr: %x" % addr, label)
            addr += label
            param = self.read_mem(addr, 0x0000, 1)
#            print("addr: %x" % addr, "param %x" % param[0])
            p = param[0] & 7

            #adat = bytearray(data)
            raw = from_bcd(data)

#            print("lab", label, "decmials:", p, "data:", data.hex())
            if p == 1 or p == 0:
                p = 4
            elif p == 2 or p == 3:
                p = 3
                if raw % 10 == 0:
                    raw = int(raw / 10)
            else:
                pass
#           print("decoded bcd:", raw)

            return raw / (10**p)


        else:
            raise Exception("not implemented yet, type/data: %d %s" % (t, repr(data)))


    def write_mem(self, addr, seg, data, chunk=0x20):

        if self.verbose:
            print("Writing %04x:%04x len 0x%x in %d byte chunks" % (seg, addr, len(data), chunk), end='')
            sys.stdout.flush()

        widx = 0
        wlen = len(data)
        while True:
            wlen = len(data) - widx
            if wlen > chunk:
                wlen = chunk


            if self.verbose:
                print(".", end ="")
                sys.stdout.flush()

            #print("Writing %04x:%04x len 0x%x" % (seg, addr+widx, wlen))
            d = struct.pack("<HH", addr+widx, seg) + data[widx:widx+wlen]
            #print("w", d.hex())

            self.gdmif.send(135, d)

            widx += wlen

            #time.sleep(0.010)

            if widx >= len(data):
                break

        if self.verbose:
            print("ok")

        return True

    def read_mem(self, addr, seg, in_rlen, chunk=0x20):
        out = bytearray()

        if seg > 0:
            raise Exception("i dont know how to read seg > 0")

        if addr + in_rlen > 0x10000:
            raise Exception("address + read length > 0x10000")

        self.write_label(160, 64)

        if self.verbose:
            print("Reading %04x:%04x len 0x%x in %d byte chunks" % (seg, addr, in_rlen, chunk), end='')

        ridx = 0
        retry = 0
        while True:
            try:
                rlen = in_rlen
                rlen = min(chunk, in_rlen)

                # we need to do value conversion manually here
                ra = addr + ridx
                rs = seg + int(ra / 0x10000) * 0x1000
                ra = ra % 0x10000
                # segment + address + count will not fit into gdm's internal variable (2*16bit) :-(
                #val = (rs * 0x10000 + ra) * 10000 + rlen*10 
                val = ra * 10000 + rlen*10 
                val = to_bcd(val)

                self.gdmif.send(252, val) 


                ret, inlab, indata = self.gdmif.recv()

#            print("r", in_rlen, rlen, len(indata), indata.hex())
                if self.verbose:
                    print(".", end ="")
                    sys.stdout.flush()
                
                indata = bytearray(indata)
                indata.reverse()
                out += indata

                ridx += rlen
                in_rlen -= rlen

#                time.sleep(0.010)

                if in_rlen <= 0:
                    break

                retry = 0
            except Exception as e:
                print("error reading, retry")
                retry += 1
                if retry > 3:
                    break

                continue

        if self.verbose:
            if in_rlen == 0:
                print("done")

        return out


    def load_defaults(self):
        for lab, d in self.labels.items():
            if d[3] is not None:
                #print(lab, d[3])
                self.write_label(lab, d[3])
                #time.sleep(0.1)

    def backup_labels(self, start, stop):
        do = OrderedDict()
        for lab, d in self.labels.items():
            if lab < start or lab >= stop:
                continue 
            if d[0] in [self.TYPE_WHAT_1, self.TYPE_WHAT_2]:
                continue

            try:
                l = self.read_label(lab)
                do[lab] = l
            except:
                pass

        return do

    def load_timedate(self):
        now = datetime.now()
        nowdate = now.strftime("%Y.%m%d")
        nowtime = now.strftime("%H.%M%S")

        self.write_label(51, nowdate)
        self.write_label(52, nowtime)

        # set battery life for 3 years
        bat = now + relativedelta(years=3)
        batdate = bat.strftime("%Y.%m%d")
        self.write_label(105, batdate)


    def load_label_names(self, label_names):
        raise Exception("select correct device")

    def backup_label_names(self):
        raise Exception("select correct device")

    def load_texts(self, texts):
        raise Exception("select correct device")

    def backup_texts(self):
        raise Exception("select correct device")
        
    def load_edm_coefs(self, coef_type, edm_bin=None):
        raise Exception("select correct device")

    def backup_edm_coefs(self):
        raise Exception("select correct device")

    def load_angle_coefs(self, angle0_bin, angle1_bin):
        raise Exception("select correct device")

    def backup_angle_coefs(self):
        raise Exception("select correct device")

    def load_servo_coefs(self, s1_bin, s2_bin):
        raise Exception("select correct device")

    def backup_servo_coefs(self):
        raise Exception("select correct device")

    def load_license(self, station_type):
        raise Exception("select correct device")
