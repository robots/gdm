import sys
import time

mem_main = bytearray(0x100000)
max_addr = 0

with open(sys.argv[1], "r") as fin:
    while True:
        dat = fin.readline()
        if len(dat) == 0:
            print("done")
            break

#        print(dat)


        if "H  " not in dat:
            print("brr", dat)
            continue

        addrs, data = dat.split("H  ")
        segs, offs = addrs.split(":")
        
        addr = (int(segs, 16) << 4) + int(offs, 16)
        print(addr, len(data), data)


        for i in range(0, 16):
    #        print(i, data[3*i:3*i+3])
            mem_main[addr+i] = int(data[3*i:3*i+3],16)
        if max_addr < addr+16:
            max_addr = addr+16

with open(sys.argv[2], "wb") as fout:
    fout.write(mem_main[:max_addr])
