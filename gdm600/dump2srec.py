import sys

mem_main = bytearray(b'\xff' * (0x8000))
bank = 0
banks = []

for i in range(0,8):
    banks.append(bytearray(b'\xff' * (0x8000)))

with open(sys.argv[1], "r") as fin:
    while True:
        l = fin.readline()
        if len(l) == 0:
            break

        if "," not in l:
            dat = l
        else:
            ts, dat = l.split(",")

        if "Bank #" in dat:
            bank+=1
            continue

        if "H:" not in dat:
            print("brr")
            raise Exception("brr")

        addrs, data = dat.split("H:")
        addr = int(addrs, 16)
#        print(addr, len(data), data)

        if addr >= 0x8000:
            addr -= 0x8000

            for i in range(0, 16):
                banks[bank][addr+i] = int(data[3*i:3*i+3],16)
            continue
        else:
            for i in range(0, 16):
        #        print(i, data[3*i:3*i+3])
                mem_main[addr+i] = int(data[3*i:3*i+3],16)


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

#with open("mainmem.bin", "wb") as fout:
#    fout.write(mem_main)
#
#
#with open("maincode.bin", "wb") as fout:
#    fout.write(mem_main[0x2000:])
#
#for i in range(0,8):
#    with open("bank%d.bin" % i, "wb") as fout:
#        fout.write(banks[i])

with open("fw.s", "wt", newline="\n") as fout:
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
    print("%X" % addr, s)
    fout.write(s)
    fout.write('\n')

