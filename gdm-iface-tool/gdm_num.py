
import struct
import math

class gdm_num:
    # 5E-10 (/ 2**64)
    epsilon = 0x0000000225c17d04

    def from_conv(num, shift, flag):
        num = num << 32

        epsilon = gdm_num.epsilon
        sign = 1
        if flag == 0x80:
            sign = -1

        if shift < 0:
            shift = -shift
            num = num >> shift
            shift = 0

        if shift > 0:
            num = num << shift
            epsilon = epsilon << shift
            shift = 0


        
        print("x %x %x %x" % (num, shift, flag))
        # + 0.00..005
        num += epsilon
        return sign * round(num / 2**64, 7)

    def to_conv(f):
        flag = 0

        if f < 0:
            f = -f
            flag = 0x80

        shift = 0
        
        num = int(round(f,7) * 2**32)

        # TODO: epsilon?

        if num > 0:
            while num > 0x80000000:
                num >>= 1
                shift += 1

            while num < 0x80000000:
                num <<= 1
                shift -= 1

        return num, shift, flag

    def from_6num(b):
        inum = [0,0]
        inum[0], inum[1], shift, flag = struct.unpack("<HHbB", b)

        num = ((inum[1] << 16) | inum[0])

        return gdm_num.from_conv(num, shift, flag)


    def to_6num(f):
        onum = [0,0]
        num, shift, flag = gdm_num.to_conv(f)
        onum[0] = (num) & 0xffff
        onum[1] = (num >> 16) & 0xffff

        return struct.pack("<HHbB", onum[0], onum[1], shift, flag)

if __name__ == "__main__":
    #f = gdm_num.from_6num(b'\x5c\x8f\xc2\xf5\xfd\x00')
    num = b'\x00\x00\x40\x9c\x0e\x00'
    print("num", num)
    f = gdm_num.from_6num(num)
    print("f", f)
    b = gdm_num.to_6num(f)
    print("b", b)
    x = gdm_num.from_6num(b)
    print("comp", x,f)

    b = gdm_num.to_6num(0.12)
    print("b", b)
    x = gdm_num.from_6num(b)
    print("comp", x,f)
