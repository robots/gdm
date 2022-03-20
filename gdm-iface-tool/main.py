#!/usr/bin/python

import os
import os.path
import threading
import time
import queue
import serial
import struct
import sys
import traceback
import socket
import math

from gdmif import GDMIf
from gdm import GDM
from gdm220 import GDM220
from gdm400 import GDM400
from gdm4000 import GDM4000

import readline
import argparse
import select

parser = argparse.ArgumentParser(description='gdm-iface python tool')

parser.add_argument('--port', dest='port', action='store', type=str, default="/dev/ttyACM0", help='Port of the gdmif')
parser.add_argument('--debug', dest='debug', action='store_const', const=True, default=False, help='Enable debug output')
parser.add_argument('--script', dest='script', action='store', type=str, default=None, help='Script to execute')

args = parser.parse_args()

workFlag = True


##crcfn = crcmod.predefined.mkPredefinedCrcFun('crc-32-mpeg')

gdms = {
    "220": GDM220,
    "400": GDM400,
    "4000": GDM4000,
}

def my_print(text):
    sys.stdout.write('\r'+' '*(len(readline.get_line_buffer())+2)+'\r')
    print(text)
    sys.stdout.write('> ' + readline.get_line_buffer())
    sys.stdout.flush()

class InputThread(threading.Thread):
    def __init__(self, threadID, name, q, gdmif, histfile = "gdmif-history", debug=False):
        threading.Thread.__init__(self)
        self.q = q
        self.name = name
        self.threadID = threadID
        self.gdmif = gdmif
        self.debug = debug

        self.histfile = histfile

        self.macros = {}
        self.level = 0

        self.script_file = None
        self.interactive = True

        self.in_macro = False
        self.cur_macro = None

        self.gdm = GDM(gdmif)
        try:
            readline.read_history_file(self.histfile)
        except IOError:
            pass

    def script_mode(self, script_file):
        self.script_file = script_file
        self.interactive = False

    def do_exit(self, force = False):
        global workFlag

        if force == False:
            self.macro_exec("at_exit") 

        readline.write_history_file(self.histfile)

        workFlag = False
        self.q.put({'data': 'exit'})

    def macro_exec(self, macro):
        if not macro in self.macros.keys():
            return

        if self.debug:
            print("Executing macro: %s" % macro)

        self.level += 1
        if self.level > 5:
            raise Exception("Too deep")

        for mcmds in self.macros[macro]:
            self.handle_cmd(mcmds)

        self.level += -1

    def handle_cmd(self, line):
        global workFlag

        cmd = line.split(' ')

        if cmd[0] == 'def' and not self.in_macro:
            mac = []
            self.cur_macro = mac
            self.macros[cmd[1]] = mac
            self.in_macro = True
            if self.debug:
                print("New macro: %s" % cmd[1])
            return
        elif cmd[0] == 'end' and self.in_macro:
            if self.debug:
                print("End of macro")
            self.in_macro = False
            self.cur_macro = None
            return

        if self.in_macro:
            if self.debug:
                print("appending to macro: %s" % line)
            self.cur_macro.append(line)
            return

        if cmd[0] in self.macros.keys():
            self.macro_exec(cmd[0])
        elif cmd[0] == 'exit':
            self.do_exit()
            if self.debug:
                print("exit!")
        elif cmd[0] == 'run':
            self.do_file(cmd[1])
        elif cmd[0] == 'sleep':
            t = float(cmd[1])
            time.sleep(t)
        elif cmd[0] == 'interactive':
            self.interactive = True
        elif cmd[0] == 'gdm':
            model = cmd[1]
            if not model in gdms.keys():
                raise Exception("no such model")

            self.gdm = gdms[model](self.gdmif)
#gdmif commands
        elif cmd[0] == 'id':
            print("ID '%s'" % (self.gdmif.get_id()))
        elif cmd[0] == 'ping':
            r = self.gdmif.ping()
            if r:
                print("Ping OK")
            else:
                print("Ping Not ok")
        elif cmd[0] == 'selftest':
            ret = self.gdmif.selftest()
            if not ret:
                print("selftest ok")
            else:
                print("bad %04x" % ret)
        elif cmd[0] == 'pwm':
            ch = int(cmd[1])
            pwm = int(cmd[2])

            self.gdmif.pwm(ch, pwm)
        elif cmd[0] == 'set_timing':
            timeout = int(cmd[1])
            bitdelay = int(cmd[2])
            deb = int(cmd[3])
            ret = self.gdmif.set_timing(timeout, bitdelay, deb)
            if ret:
                print("ok")
            else:
                print("bad")
        elif cmd[0] == 'recv':
            ret = self.gdmif.recv()
            if ret[0] == 0:
                print("ok, lab: %d data: %s" % (ret[1], ret[2].hex()))
            else:
                print("bad %04x" % ret[0])
        elif cmd[0] == 'send':
            if len(cmd) < 3:
                raise Exception("send label data")
            lab = int(cmd[1])
            data = eval(cmd[2])

            ret = self.gdmif.send(lab, data)
            if ret == True:
                print("ok")
            else:
                print("bad %04x" % ret)
#gdmspecific commands
        elif cmd[0] == 'rmem':
            if len(cmd) < 3:
                raise Exception("rmem seg:addr len [file]")
            p = cmd[1].split(':')
            addr = int(p[1], 16)
            seg = int(p[0], 16)
            l = int(cmd[2])
            fname = False
            doread = True

            if len(cmd) > 3:
                fname = cmd[3]

                if os.path.isfile(fname):
                    if True:
                        raise Exception("File already exists")
                    else:
                        s = os.stat(fname)
                        l = l - s.st_size
                        addr += s.st_size
                        seg = int (addr / 0x10000) * 0x1000
                        addr = addr % 0x10000
                        print("File exists, resuming download")

                        if l == 0:
                            print("Download len = 0, all done")
                            doread = False

            if doread:
                data = self.gdm.read_mem(addr, seg, l)
                
                if fname:
                    with open(fname, 'wb') as fout:
                        fout.write(data)
                else:
                    print(data.hex())
        elif cmd[0] == 'wmem':
            if len(cmd) < 3:
                raise Exception("wmem seg:addr data")
            p = cmd[1].split(':')
            addr = int(p[1], 16)
            seg = int(p[0], 16)
            data = eval(cmd[2])
            ret = self.gdm.write_mem(addr, seg, data)
            if not ret == True:
                print("bad %04x" % ret)
        elif cmd[0] == 'wmem_file':
            if len(cmd) < 3:
                raise Exception("wmem_file seg:addr file")
            p = cmd[1].split(':')
            addr = int(p[1], 16)
            seg = int(p[0], 16)

            with open(fname, "rb") as fin:
                data = fin.read()

            if len(data) > 250:
                raise Exception("data len problem", len(data))

            ret = self.gdm.write_mem(addr, seg, data)
            if not ret == True:
                print("bad %04x" % ret)
        elif cmd[0] == 'rlab':
            lab = eval(cmd[1])
            if isinstance(lab, list):
                l = []
                for i in lab:
                    val = self.gdm.read_label(i)
                    l.append(val)
                print(l)
            else:
                val = self.gdm.read_label(lab)
                print("Label", lab, " value:", val)
        elif cmd[0] == 'wlab':
            print("Writing label", int(cmd[1]), ":", cmd[2]) 
            self.gdm.write_label(int(cmd[1]), cmd[2])

        elif cmd[0] == 'gdm_load_edm':
            edmtype = int(cmd[1])
            edmbin = None
            if len(cmd) > 2:
                edmfile = cmd[2]
                with open(a1file, 'rb') as fin:
                    edmbin = fin.read()
            self.gdm.load_edm_coefs(edmtype, edmbin)

        elif cmd[0] == 'gdm_load_angle':
            a1file = cmd[1]
            a2file = cmd[2]
            with open(a1file, 'rb') as fin:
                a1coef = fin.read()
            with open(a2file, 'rb') as fin:
                a2coef = fin.read()

            self.gdm.load_angle_coefs(a1coef, a2coef)
        elif cmd[0] == 'gdm_load_angle_text':
            a1file = cmd[1]
            a2file = cmd[2]

            self.gdm.load_angle_coefs_text(a1file, a2file)

        elif cmd[0] == 'gdm_load_label_names':
            labfile = cmd[1]
            with open(labfile, 'rb') as fin:
                lab = fin.read()
            self.gdm.load_label_names(lab)

        elif cmd[0] == 'gdm_lic':
            lic = int(cmd[1])
            self.gdm.load_license(lic)
        elif cmd[0] == 'gdm_load_defaults':
            self.gdm.load_defaults()
        elif cmd[0] == 'gdm_timedate':
            self.gdm.load_timedate()
        elif cmd[0] == 'gdm_texts':
            t1f = cmd[1]
            t2f = cmd[2]
            t = []
            with open(t1f, 'rb') as fin:
                t.append(fin.read())
            with open(t2f, 'rb') as fin:
                t.append(fin.read())

            self.gdm.load_texts(t)
        elif cmd[0] == 'backup_labels':
            start = int(cmd[1])
            stop = int(cmd[2])
            fo = cmd[3]
            d = self.gdm.backup_labels(start, stop)

            with open(fo, "wt") as fout:
                for label, value in d.items():
                    out = ""
                    out += "%d=" % label
                    out += str(value)
                    out += "\n"

                    fout.write(out)
        elif cmd[0] == 'backup_texts':
            fname = cmd[1]
            texts = self.gdm.backup_texts()

            for i, t in enumerate(texts):
                with open("%s_%d" % (fname, i), 'wb') as fo:
                    fo.write(t)

        elif cmd[0] == 'backup_edm_coefs':
            fname = cmd[1]
            edm = self.gdm.backup_edm_coefs()

            for i, t in enumerate(edm):
                with open("%s_%d" % (fname, i), 'wb') as fo:
                    fo.write(t)
        elif cmd[0] == 'backup_angle_coefs':
            fname = cmd[1]
            angle = self.gdm.backup_angle_coefs()

            for i, t in enumerate(angle):
                with open("%s_%d" % (fname, i), 'wb') as fo:
                    fo.write(t)
        elif cmd[0] == 'backup_servo_coefs':
            fname = cmd[1]
            servo = self.gdm.backup_servo_coefs()

            for i, t in enumerate(servo):
                with open("%s_%d" % (fname, i), 'wb') as fo:
                    fo.write(t)
        elif cmd[0] == 'backup_label_names':
            fname = cmd[1]
            lbl = self.gdm.backup_label_names()
            
            for i, t in enumerate(lbl):
                with open("%s_%d" % (fname, i), 'wb') as fo:
                    fo.write(t)

        else:
            if not cmd[0] == '':
                print("unknown command", cmd)
                #self.q.put({'data': line})

    def do_file(self, filename):
        with open("gdmif-script", "r") as configfile:
            for line in configfile:
                line = line.strip()

                if len(line) == 0:
                    continue

                if line[0] == '#':
                    continue

                #print repr(line)
                self.handle_cmd(line)

    def run(self):
        global workFlag

        self.do_file("gdmif-script")

        if self.in_macro:
            raise Exception("missing macro termination")

        print("Loaded macros: %s" % self.macros.keys())

        if self.script_file is not None:
            self.do_file(self.script_file)

        if self.interactive:
            # interactive mode
            while workFlag:
                try:
                    data = input('>')

                    self.handle_cmd(data)

                except (EOFError, KeyboardInterrupt) as e:
                    #self.do_exit()
                    break
                except:
                    print(traceback.format_exc())
                    print("Error", sys.exc_info()[0])

        self.do_exit()


threads = []
cmdQueue = queue.Queue(10)

# initialize threads
gdmif = GDMIf(args.port)
gdmif.start()

threads.append(gdmif)

gdmifid = gdmif.get_id()

if gdmifid is False:
    workFlag = False
    print("no gdmif found")
else:
    print("gdmif ID '%s'" % gdmifid )

    thinp = InputThread(2, "Input", cmdQueue, gdmif, debug=args.debug)
    if args.script is not None:
        thinp.script_mode(args.script)
    thinp.start()
    threads.append(thinp)

# wait for ctrl+c
try:
    while workFlag:
        time.sleep(0.1)
except KeyboardInterrupt:
    workFlag = False

gdmif.stop()

print("Waiting for threads to join")
for t in threads:
    t.join()

