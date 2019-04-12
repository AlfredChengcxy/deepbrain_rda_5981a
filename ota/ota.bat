python image_pack_firmware.py ../../../../build/UNO_81C/ARM/demo.bin 1.0.2.0 0x18004000 0x0
python image_merge.py bootloader.bin 0x1000 ../../../../build/UNO_81C/ARM/demo_fwpacked.bin 0x3000 demo_merge.bin
pause 