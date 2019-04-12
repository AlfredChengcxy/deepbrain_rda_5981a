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
#define FIRMWARE_MAGIC      0xEAEA
struct firmware_info
{
    uint32_t magic;
    uint8_t  version[VERSION_SZ];

    uint32_t addr;
    uint32_t size;
    uint32_t crc32;
    uint32_t bootaddr;    //new
};
"""

def pack_image(filename, version, firmware_addr, bootaddr):

    fname = os.path.splitext(filename)

    print 'firmware:', filename

    f = file(filename, 'rb')
    data = f.read()
    f.close()

    firmware_magic = 0xEAEA
    #firmware_addr = 0x18004000

    #ISOTIMEFORMAT='%Y-%m-%d %X'
    #version = time.strftime( ISOTIMEFORMAT, time.localtime() )

    crc32 = zlib.crc32(data, 0) & 0xFFFFFFFF
    crc32 ^= 0xFFFFFFFF
    size = len(data)
    print '    size:', size
    print '   version:', version
    print '   crc32: %08x' % crc32

    if bootaddr >= 0x18001000 and bootaddr < 0x18400000:
    	print 'bootaddr:%08x' % bootaddr
    else:
    	print 'bootaddr(%08x) is invalid, or no input, disable it' % bootaddr
    	bootaddr = 0

    header = struct.pack("L24sLLLLL4048s", firmware_magic, version, firmware_addr, size, crc32, bootaddr, firmware_magic, '\0')

    f = file(fname[0] + '_fwpacked.bin', "wb")
    f.write(header)
    f.write(data)
    f.close()

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print sys.argv[0], "filename"
        exit(0)
    if len(sys.argv) == 5:
    	pack_image(sys.argv[1], sys.argv[2], int(sys.argv[3],16), int(sys.argv[4],16))
    else:
    	pack_image(sys.argv[1], sys.argv[2], int(sys.argv[3],16), 0)