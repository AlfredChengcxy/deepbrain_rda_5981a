copy ..\\..\\..\\..\\build\\UNO_81C\\ARM\\main.bin main_ota.bin
copy ..\\..\\..\\..\\build\\UNO_81C\\ARM\\main.map main_ota.map
python image_pack_firmware.py main_ota.bin 1.0.0.0 0x18004000 0x0
python image_merge.py bootloader.bin 0x1000 main_ota_fwpacked.bin 0x3000 main_merge_ota.bin
pause 