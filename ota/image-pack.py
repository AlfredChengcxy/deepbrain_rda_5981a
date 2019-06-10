import os
import sys
import struct
import zlib
import time

"""
#define IMAGE_MAGIC         0xAEAE
struct image_header
{
    uint16_t magic;
    uint8_t  encrypt_algo;
    uint8_t  resv[1];

    uint8_t  version[VERSION_SZ];

    uint32_t crc32;
    uint32_t size;
	//uint8_t  padding[4060];
};
"""

def pack_image(filename, version):

    fname = os.path.splitext(filename)

    print ('firmware:', filename)

    f = open(filename, 'rb')
    data = f.read()
    f.close()

    magic = 0xAEAE
    encrypt_algo = 0
    rescv = 0

    #ISOTIMEFORMAT='%Y-%m-%d %X'
    #version = time.strftime( ISOTIMEFORMAT, time.localtime() )

    crc32 = zlib.crc32(data, 0) & 0xFFFFFFFF
    crc32 ^= 0xFFFFFFFF
    size = len(data)
    print ('    size:', size)
    print ('   version:', version)
    print ('   crc32: %08x' % crc32)

    header = struct.pack("<HBB24sLL", magic, encrypt_algo, rescv, bytes(version.encode('utf-8')), crc32, size)

    f = open(fname[0] + '_upgrade.bin', "wb")
    f.write(header)
    f.write(data)
    f.close()

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print (sys.argv[0], "filename")
        exit(0)

    pack_image(sys.argv[1], sys.argv[2])
