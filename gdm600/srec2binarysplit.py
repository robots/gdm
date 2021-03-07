import binascii
import sys

mem_main = bytearray(b'\xff' * (0x8000))
banks = []

for i in range(0,8):
    banks.append(bytearray(b'\xff' * (0x8000)))

with open(sys.argv[1], "rt") as fin:
    for l in fin:

        if not l[0] == 'S':
            raise Exception("S not first")

        t = l[1]
        length = int(l[2:4],16)
        idx = 4
        addr = 0
        bank = -1
        if t == '0':
            # header record
            continue
        elif t == '1': # parse 1 data
            addr = int(l[idx:idx+4],16)
            idx+=4
            length -= 2
        elif t == '2': # parse 2 data
            bank = int(l[idx:idx+2],16)
            idx+= 2
            addr = int(l[idx:idx + 4],16)
            idx += 4
            length -= 3
        elif t == '9' or t == '8':
            break
        else:
            raise Exception("unsupported record")

        print(len(l), l)
        print("data %d '%s' "%(len(l[idx:idx+(length-1)*2]),l[idx:idx+(length-1)*2]))
        data = binascii.unhexlify(l[idx:idx+(length-1)*2])
        print("bank %d addr %x" % (bank, addr))

        if bank > 0:
            for i in range(len(data)):
                banks[bank][addr-0x8000+i] = data[i]
            continue
        else:
            for i in range(len(data)):
        #        print(i, data[3*i:3*i+3])
                mem_main[addr+i] = data[i]


with open("mainmem.bin", "wb") as fout:
    fout.write(mem_main)

with open("maincode.bin", "wb") as fout:
    fout.write(mem_main[0x2000:])

for i in range(0,8):
    with open("bank%d.bin" % i, "wb") as fout:
        fout.write(banks[i])

