
import serial
import time
import sys
import re

serport = "COM5"
firmwareoutput = "fw.s"

if len(sys.argv) > 1:
    serport = sys.argv[1]
if len(sys.argv) > 2:
    firmwareoutput = sys.argv[2]

mem_main = bytearray(b'\x00' * (0x8000))
bank = 0
banks = []

for i in range(0,8):
    banks.append(bytearray(b'\x00' * (0x8000)))


def srec(t, a, d = None):
    chksum = 0
    out = "S%d" % t

    if t == 9:
        l = 2 + 1
    elif t == 8:
        l = 3 + 1
    elif t == 1:
        l = 2 + len(d) + 1
    elif t == 2:
        l = 3 + len(d) + 1

    out += "%02X" % l
    chksum += l

    if len(out) != 4:
        raise Exception("zle")

    if t in [1, 9]:
        s = "%04X" % a
        if len(s) != 4:
            print("addr %x" % a)
            raise Exception("zle")
        out += s
        chksum += (a >> 8) & 0xff
        chksum += (a) & 0xff
    elif t in [2, 8]:
        s = "%06X" % a
        if len(s) != 6:
            print("addr %x" % a)
            raise Exception("zle")
        out += s
        chksum += (a >> 16) & 0xff
        chksum += (a >> 8) & 0xff
        chksum += (a) & 0xff

    if t in [1,2]:
#        print(l, repr(d))
        for i in range(len(d)):
            s = "%02X" % d[i]
            if len(s) != 2:
                print("%x" % d[i])
                raise Exception("zle")
            out += s
            chksum += d[i]

    chksum = 0xff - (chksum & 0xff)
    out += "%02X" % chksum

    return out

def gdm_send(cmd):
    print("send", cmd)
    ser.write(cmd.encode('ascii'))

def gdm_write(cmd, label, value):
    out = "W%c,%d=%d\r" % (cmd, label, value)

    print("cmd send:", out)
    ser.write(out.encode('ascii'))
    l = ser.readline().decode('ascii')
    print("cmd recv:", l)

    val = re.findall('\d*\.?\d+', l)
    
    return val[0]

def gdm_read(cmd, label):

    out = "R%c,%d\r" % (cmd, label)

    print("cmd send:", out)
    ser.write(out.encode('ascii'))
    l = ser.readline().decode('ascii')
    print("cmd recv:", l)

    if '=' not in l:
        raise Exception("bad response")

    p, val = re.findall('\d*\.?\d+', l)

    if int(p) != label:
        raise Exception("bad response param")

    return val


ser = serial.Serial(serport, 9600, timeout = 1, rtscts=0, dsrdtr=0)

seed = gdm_read('G', 127)
seed = int(seed)
print("seed", seed)
key = ((seed+171)*0x0d)-780
print("key", key)
key = key & 0xffff
print("key", key)
response = gdm_write('G', 127, key)
if not response == '0.0':
    raise Exception("bad key")


gdm_send("OV*\r")

if True:
    while True:
        dat = ser.readline()
        if len(dat) == 0:
            sys.stdout.write('\n')
            break

        dat = dat.decode('ascii')
#        print(dat)

        if "Bank #" in dat:
            bank+=1
            sys.stdout.write('\n')
            continue

        if "H:" not in dat:
#            print("brr", dat)
            continue

        addrs, data = dat.split("H:")
        addr = int(addrs, 16)
#        print(addr, len(data), data)


        if len(data) < 16*3:
            print("brr1",dat )
            continue

        if addr >= 0x8000:
            addr -= 0x8000

            for i in range(0, 16):
                banks[bank][addr+i] = int(data[3*i:3*i+3],16)
            sys.stdout.write("\rReading bank %i/8 ... %04x %d%%" % (bank, addr+0x8000, int(addr*100/0x8000)))
        else:
            for i in range(0, 16):
        #        print(i, data[3*i:3*i+3])
                mem_main[addr+i] = int(data[3*i:3*i+3],16)

            sys.stdout.write("\rReading main mem ... %04x %d%%" % (addr, int(addr*100/0x8000)))

        sys.stdout.flush()

print("Readout done. Saving to firmware to ", firmwareoutput)

with open(firmwareoutput, "wt") as fout:
    for i in range(0x20, 0x8000, 16):
        data = mem_main[i:i+16]
        if sum(data) == 0:
            continue
        s = srec(1, i, data)
#        print("%x" % i, s)
        fout.write(s)
        fout.write('\n')

    for b in range(1,8):
        for i in range(0, 0x8000, 16):
            addr = (b << 16) + i + 0x8000
            data = banks[b][i:i+16]
            if sum(data) == 0:
                continue
            s = srec(2, addr, data)
#            print("%x" % addr, s)
            fout.write(s)
            fout.write('\n')

    s = srec(9, 0)
#    print("%X" % addr, s)
    fout.write(s)
    fout.write('\n')

response = gdm_write('G', 127, key+1)
