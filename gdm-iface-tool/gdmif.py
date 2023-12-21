#!/usr/bin/env python

import threading
import sys
import struct
import serial
import crcmod
import time

class GDMIfSub:

    def __init__(self, gdmif, if_idx):
        self.gdmif = gdmif
        self.if_idx = if_idx
        self.pkt_handler = None

        self.gdmif.register_sub(if_idx, self)

    def set_mode(self, mode):
        return self.gdmif.set_mode(self.if_idx, mode)

    def set_timing(self, timeout, bitdelay, deb):
        return self.gdmif.set_timing(self.if_idx, timeout, bitdelay, deb)

    def send(self, label, buf):
        return self.gdmif.send(self.if_idx, label, buf)

    def recv(self):
        return self.gdmif.recv(self.if_idx)

    def register_pkt_handler(self, handler):
        self.pkt_handler = handler

    def process_pkt(self, buf):
        print(f"{self.if_idx}: {repr(buf)}")
        if self.pkt_handler is not None:
            self.pkt_handler(buf)

class GDMIf(threading.Thread):

    CMD_ID = 0x01
    CMD_PING = 0x02
    CMD_SELFTEST = 0x03
    CMD_RESET = 0x04
    CMD_DEBUG = 0x05
    CMD_SNIFPKT = 0x06
    CMD_TIMEOUT = 0x07

    CMD_PWM = 0x10

    CMD_MODE = 0x20
    CMD_SEND = 0x21
    CMD_RECV = 0x22

    CMD_PKT = 0x70

    timeout = 1

    def __init__(self, serialport, debug = False):
        threading.Thread.__init__(self)
        self._transfer = None
        self.freq = 0
        self.crcfn = crcmod.predefined.mkPredefinedCrcFun('crc-32-mpeg')
        self.debug = debug

        self.cmd_semaphore = threading.Semaphore(0)
        self.cmd_sent = False 

        self.subif = {}

        self.serial_hrd = struct.Struct("=BH")

        self.open_serial(serialport)
        self.workFlag = True
    
    def register_sub(self, idx, obj):
        self.subif[idx] = obj

    def stop(self):
        self.workFlag = False

    def open_serial(self, port):
        self.serial = serial.Serial(port, 921600, timeout = 10, rtscts = 0, dsrdtr = 0)
        self.serial.inter_byte_timeout = 10
        self._transfer = self._send_cmd

    def process_serial(self):
        r = self.serial.read(1)
        if len(r) == 0:
            return False

        if struct.unpack("B", r)[0] != 0xaa:
            return False

        hdr = self.serial.read(3)
        if not len(hdr) == 3:
            print("Timeout reading hdr")
            return False
#        print(repr(hdr))
        reccmd, reclen = self.serial_hrd.unpack(hdr)
        recdata = None
#        print(reccmd, reclen)
        if reclen == 0xffff:
#            print("Received NAK")
            recdata = False
            crc_calc = self.crcfn(hdr)
        elif reclen == 0:
#            print("Received ACK")
            recdata = True
            crc_calc = self.crcfn(hdr)
        else:
            recdat = self.serial.read(reclen)
            if len(recdat) < reclen:
                print("Timeout reading data", len(recdat), reclen)
                return False
#           print("Received DATA: ", recdat.hex())
            recdata = recdat
            crc_calc = self.crcfn(hdr + recdata)

        crcraw = self.serial.read(4)
        if len(crcraw) < 4:
#            print("Timeout reading crc")
            return False
        crc = struct.unpack(">I", crcraw)[0]

        #check crc;
        if not crc == crc_calc:
#            print("crc check errro %08x %08x" % (crc, crc_calc))
            return False

        if not reccmd & 0x80 == 0x80:
            print("unknown command recvd %02x" % reccmd)

        reccmd = reccmd & 0x7f

        return {'cmd':reccmd, 'data':recdata}

    def run(self):
        while self.workFlag:
            if self.serial.inWaiting():
#                print("check serial")
                data = self.process_serial()
                if self.debug:
                    print("debug %s %s" % (self.name, data))

#                print(data)
                if data is False:
                    continue

                if data['cmd'] == GDMIf.CMD_PKT:
                   iface = data['data'][0]
                   if iface in self.subif:
                       self.subif[iface].process_pkt(data['data'][1:])
                if data['cmd'] == GDMIf.CMD_SNIFPKT:
                    my_print ("sniff pkt")
                elif data['cmd'] == GDMIf.CMD_DEBUG:
                    my_print("DEBUG: Len = %d Data: %s" % (len(data['data']), repr(data['data'])))
                else:
                    pass
#                    my_print("%x" % data['cmd'])

                if not self.cmd_sent == False:
                    if data['cmd'] == self.cmd_sent:
                        self.cmd_sent = False
                        self.cmd_resp = data['data']
#                        print("cmd_resp = ", self.cmd_resp, "releasing semaphore")
                        self.cmd_semaphore.release()
#                else:
#                    print("ssaass", data['cmd'])

            else:
                time.sleep(0.001)


    def _send_cmd(self, cmd, data = b'', send=True):
        out = struct.pack("<BH", cmd, len(data)) + data
        out += struct.pack("<I", self.crcfn(out))

        out = struct.pack("B", 0xAA) + out

        self.cmd_sent = cmd
        if self.debug:
            print("sent", cmd)
        if send:
#            print("sent data %s " % (repr(out)))
            self.serial.write(out)

        t = time.time()
        while True:
            if self.cmd_semaphore.acquire(False):
                break

            if time.time() - t > self.timeout: #1:
                if self.debug:
                    print("Timeout")
                self.cmd_sent = False
                return False

            time.sleep(0.0001)

#        print("cmd ok!")
        return self.cmd_resp

    def _serial_transfer(self, cmd, data = "", send = True):
        if send:
            out = struct.pack("<BH", cmd, len(data)) + data
            out += struct.pack("<I", self.crcfn(out))

            out = struct.pack("B", 0xAA) + out

            self.serial.write(out)

        r = self.serial.read(1)

        # get sof
        if len(r) == 0:
            raise Exception("nothing")

        if struct.unpack("B", r)[0] != 0xaa:
            raise Exception("Not sof")

        # get cmd
        r = self.serial.read(1)
        if struct.unpack("B", r)[0] != (cmd | 0x80):
            raise Exception("wrong data received %d != %d", struct.unpack("B",r)[0], cmd)

        # get len
        r = self.serial.read(2)
        reclen = struct.unpack("<H", r)[0] 

        recdat = False
        if reclen == 0xffff:
            recdat = False
        elif reclen == 0:
            recdat = True
        else:
            recdat = self.serial.read(reclen)

        r = self.serial.read(4)
        crc, = struct.unpack("<I", r)
        # check crc

        return recdat

    def wait_for(self, cmd):
        return self._transfer(cmd, send=False)


    def get_id(self):
        r = self._transfer(self.CMD_ID)
        return r.decode('ascii')

    def ping(self, string = b'ahoj'):
        ret = self._transfer(self.CMD_PING, string)

        if ret == string:
            return True

        return False

    def selftest(self):
        ret = self._transfer(self.CMD_SELFTEST)

        if isinstance(ret, bool):
            return False

        r, = struct.unpack("<I", ret)
        return r
 
    # pwm 2000-4000 = 1ms - 2ms
    def pwm(self, ch, pwm):
        data = struct.pack("<BH", ch, pwm)
        return self._transfer(self.CMD_PWM, data)

    def set_timing(self, ifaceidx, timeout, bitdelay, deb):
        if timeout <= bitdelay * deb:
            raise Exception("timeout <= bitdelay * deb")

        data = struct.pack("<BHHBBH", ifaceidx, timeout, bitdelay, deb, 0, 0)

        ret = self._transfer(self.CMD_TIMEOUT, data)

        if ret == True:
            self.timeout = timeout / 1000

        return ret

    def set_mode(self, ifaceidx, mode):
        data = struct.pack("<BB", ifaceidx, mode)
        return self._transfer(self.CMD_MODE, data)

    def send(self, ifaceidx, label, buf):
        data = struct.pack("<BB", ifaceidx, label) + buf

        if self.debug:
            print("send", data.hex())

        r = self._transfer(self.CMD_SEND, data)
        if isinstance(r, bool):
            raise Exception("timeout sending")
            #return False

        ret, = struct.unpack("<I", r[0:4])
        if not ret == 0:
            raise Exception("error sending %04x" % ret)

        return True

    def recv(self, ifaceidx):
        dat = self._transfer(self.CMD_RECV, struct.pack("<B", ifaceidx))

        if isinstance(dat, bool):
            raise Exception("timeout receiving")
            #return 0xffff, None, None

        ret, label = struct.unpack("<IB", dat[:5])
        if ret > 0:
            raise Exception("error receiving %04x" % ret)
            return ret, None, None

        return ret, label, dat[5:]



if __name__ == "__main__":
    s = GDMIf("/dev/ttyACM0")
    s.start()

    print("ID '%s'" % (s.get_id()))
    print("ID '%s'" % (s.get_id()))

    s.stop()
