# -*- coding:utf-8 -*-
#__author = zhangguohua02

import time
import os
import os.path
import shutil
import re
from pprint import pprint


def getCurrentTime():
    nowTime = time.localtime()
    year = str(nowTime.tm_year)
    month = str(nowTime.tm_mon)
    day = str(nowTime.tm_mday)
    hour = str(nowTime.tm_hour)
    min = str(nowTime.tm_min)
    sec = str(nowTime.tm_sec)
    if len(month) < 2:
        month = '0' + month
    if len(day) < 2:
        day = '0' + day
    if len(min) < 2:
        min = '0' + min
    if len(sec) < 2:
        sec = '0' + sec
    return year + '-' +  month + '-' + day + '-' + hour + '-' + min + '-' + sec


"""
search_dir is the search directory, the function will search the subdirectory too.
fileName is the key, the function support regular express when searching.
copy_dir is the directory which matched-file will copy into.
"""
def search_and_copy_file_by_name(search_dir, file_name, copy_dir):
    if not os.path.exists(search_dir):
        print ('search_dir not exit')
        return -1
    for file in os.listdir(search_dir):
        target_file = os.path.join(search_dir, file)
        #print target_file
        #print os.path.isfile(target_file)
        if os.path.isfile(target_file) and re.match(file_name, file):
            if not os.path.exists(copy_dir):
                os.makedirs(copy_dir)
            dst_file = os.path.join(copy_dir, file)
            #print dst_file
            #print target_file
            open(dst_file, "wb").write(open(target_file,"rb").read())

        if os.path.isdir(target_file):
            search_and_copy_file_by_name(target_file, file_name, copy_dir)


"""
remove the files which match the 'key' in regular express in the dir
"""
def remove_file(dir, key):
    if not os.path.exists(dir):
        print ('dir not exit')
        return -1
    for file in os.listdir(dir):
        #print file
        if re.match(key, file):
            remove_file = os.path.join(dir, file)
            os.remove(remove_file)


if __name__ == "__main__":
    dirTime = getCurrentTime()
    src_dir = 'D:\\jenkins\\workspace\\iot_trunk_compile\\IoT-OS\\projects'
    dst_dir = 'F:\\IoTImage\\'
    dst_dir = dst_dir + dirTime
    search_and_copy_file_by_name(src_dir, 'uv4_log|TinyDu*', dst_dir)
    remove_file(dst_dir, '.*?(uvoptx|uvprojx|BAT|bat)$')