

copy ..\\Build\\main.bin main_ota_1000.bin
copy ..\\Build\\main.bin main_ota_1001.bin
copy ..\\Build\\main.map main_ota.map

python image_pack_firmware.py main_ota_1000.bin 1.0.0.0 0x18006000 0x0
python image_merge.py bootloader.bin 0x1000 main_ota_1000_fwpacked.bin 0x5000 main_merge_ota.bin
python image-pack.py main_ota_1001.bin 1.0.0.8
copy main_merge_ota.bin ..\\Build\\main_merge_ota.bin
pause 