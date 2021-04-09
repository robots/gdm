
import serial
import time
import sys

firmwarefile = "fw.s"
if len(sys.argv) > 1:
    firmwarefile = sys.argv[1]

serialport = "/dev/ttyUSB0"
if len(sys.argv) > 2:
    serialport = sys.argv[2]

ser = serial.Serial(serialport, 9600, timeout = 10, rtscts=0, dsrdtr=0)

#ser.write("?\r".encode('ascii'))
ser.write("LV=0\r".encode('ascii'))
print("sending erase command, wait ...")

time.sleep(1)
ser.baudrate = 38400

l = ser.read(1)
if len(l) == 0:
    print("bad")
    sys.exit(0)
print(l)
if not l == b'*':
    print("no")

num_lines = sum(1 for line in open(firmwarefile, "rt"))
cur_line = 0

with open(firmwarefile, "rt") as fin:
    while True:
        l = fin.readline()
        if len(l) == 0:
            break

        cur_line += 1
        sys.stdout.write("\rWriting %d%% %d/%d" % (int(cur_line*100/num_lines), cur_line, num_lines))
        sys.stdout.flush()
#        print(l)

#        for i in range(len(l)):
#            ser.write(l[i].encode('ascii'))
#            time.sleep(0.001)
        ser.write(l.encode('ascii'))
        time.sleep(0.04)

        if ser.in_waiting > 0:
            r = ser.read(ser.in_waiting)
            print(r)
            if b'35.8' in r:
                raise Exception("checksum error")
            raise Exception('bad')

ser.baudrate = 9600

l = ser.readline()
if len(l) == 0:
#    print("bad")
    sys.exit(0)

