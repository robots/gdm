
import struct
import serial
import time
import sys
import os

verbose = False

TYPE_DATA=0
TYPE_EOF=1
TYPE_EXT_SEGADDR=2
TYPE_START_SEGADDR=3

firmwarefile = "fw.bin"
if len(sys.argv) > 1:
    firmwarefile = sys.argv[1]

serialport = "/dev/ttyUSB0"
if len(sys.argv) > 2:
    serialport = sys.argv[2]

def calc_key(seed, p1, p2, p3):
    k = (seed + p1) * p2 - p3
    return k & 0xffff

chunksize = 32
ichdelay = 0.001
def send_data(ser, addr, data):
    global verbose

    pkt = struct.pack(">BHB", len(data), addr, TYPE_DATA) + data
    chsum = 0x100 - (sum(pkt) & 0xff)
#    for i in range(len(pkt)):
#        ser.write(pkt[i:i+1])
#        time.sleep(ichdelay)
    ser.write(pkt)
    ser.write(struct.pack("B", chsum & 0xff))

    if verbose:
        print(pkt, "%x %x" % (chsum, sum(pkt)))


def send_eof(ser):
    pkt = struct.pack(">BHB", 0, 0, TYPE_EOF) + b'\xff'
    for i in range(len(pkt)):
        ser.write(pkt[i:i+1])
        time.sleep(ichdelay)
#    ser.write(pkt)

def send_ext_seg(ser, seg):
    pkt = struct.pack(">BHB", 2, 0, TYPE_EXT_SEGADDR) + struct.pack(">H", seg)
    chsum = 0x100 - (sum(pkt) & 0xff)
    #ser.write(pkt)
    for i in range(len(pkt)):
        ser.write(pkt[i:i+1])
        time.sleep(ichdelay)
    ser.write(struct.pack("B",chsum & 0xff))

def send_start_seqaddr(ser, addr, seg):
    pkt = struct.pack(">BHB", 4, 0, TYPE_START_SEGADDR) + struct.pack(">HH", addr, seg)
    chsum = 0x100 - (sum(pkt) & 0xff)
    ser.write(pkt)
#    for i in range(len(pkt)):
#        ser.write(pkt[i:i+1])
#        time.sleep(0.01)
    ser.write(struct.pack("B", chsum & 0xff))

ser = serial.Serial(serialport, 9600, timeout = 1, rtscts=0, dsrdtr=0)

print("turn on CU with MNU + PWR")

retry = 0
while True:
    cu_id = ser.read(100)
    #print(cu_id)
    if len(cu_id) < 1:
        retry += 1
        if retry > 10:
            print("try again")
            sys.exit(0)
    if b'>' in cu_id:
        break

print(cu_id)

# extract seed from id string:
cu_id_parts = cu_id.split(b'>')
seed = int(cu_id_parts[-2])
print("Seed:", seed)

# i dont know what other keys do, but k4 selects 115200 
# and since we are uploading whole memory - who cares.

#k0 = calc_key(seed, 0xab, 0x0d, 0x30c)
#k1 = calc_key(seed, 0xad, 0x17, 0x30c)
#k2 = calc_key(seed, 0xb3, 0x11, 0x30c)
#k3 = calc_key(seed, 0xbf, 0x0b, 0x30c)
k4 = calc_key(seed, 0xc1, 0x13, 0x30c)
#k5 = calc_key(seed, 0xc5, 0x1d, 0x30c)
key = "%i>" % k4

print("Key", key)
ser.write(key.encode('ascii'))

def eff_addr(seg, addr):
    return (seg << 4) + addr

input("Select 2. IHloader 115200 on CU, and press enter here when ready")

ser.baudrate = 115200
time_start = time.time()

with open(firmwarefile, "rb") as fin:
    fin.seek(0, os.SEEK_END)
    file_size = fin.tell()
    fin.seek(0, os.SEEK_SET)

    seg = 0
    addr = 0

    send_ext_seg(ser, seg)

    while True:
        data = fin.read(chunksize)
        if len(data) == 0:
            break

        #time.sleep(0.01)

        if addr >= 0x8000:

            addr -= 0x8000
            seg  += 0x0800

            send_ext_seg(ser, seg)

            time.sleep(0.01)

        
        # skip rom working space :-)
        ea = eff_addr(seg, addr)
        if not (ea >= 0x7000 and ea <= 0x9000):
            send_data(ser, addr, data)
            time.sleep(0.001)

        addr += len(data)

        pos = fin.tell()
        sys.stdout.write("\r%04x:%04x %d/%d = %.2f" % (seg, addr, pos, file_size, pos*100/file_size))
        sys.stdout.flush()

#        if ser.in_waiting > 0:
#            print(ser.read())
#        break


    send_eof(ser)

print("Time: %.2f sec" % (time.time()-time_start))
