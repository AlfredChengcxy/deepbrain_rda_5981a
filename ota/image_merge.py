import os
import sys
import struct
import zlib
import time
	
def full_empty(file,len):
    empty = struct.pack('B',0xFF)
    while len :
        file.write(empty)
        len = len - 1

def merge_image(file1, offset1, file2, offset2, outfile):
    #fp1 = os.path.splitext(file1)
    print 'file1:', file1
    
    f1 = file(file1, 'rb')
    data1 = f1.read()
    f1.close()
    size1 = len(data1)

    fout = file(outfile,"wb")
    fout.write(data1)

    fulllen = offset2-offset1-size1
    full_empty(fout,fulllen)
        
    print 'file2:', file2
    f2 = file(file2, 'rb')
    data2 = f2.read()
    f2.close()  

    fout.write(data2)
    fout.close()

    print 'complete.'

def merge_image_more(param):
    print 'num:',len(param)
    print 'argv:',param
    
if __name__ == "__main__":
    if len(sys.argv) < 6:
        print sys.argv[0], "filename"
        exit(0)

    if len(sys.argv) == 6:
        merge_image(sys.argv[1], int(sys.argv[2],16), sys.argv[3], int(sys.argv[4],16), sys.argv[5])

        