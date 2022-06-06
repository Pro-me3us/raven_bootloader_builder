# Raven Bootloader Builder
Modifications to Amazon script to compile a custom bootloader for the FireTV 2nd gen Cube, with U-Boot &amp; Fastboot restrictions removed.

### About
This bootloader compiler is a modification of the build script included with <a href="https://fireos-tv-src.s3.amazonaws.com/YbHeBIPhSWxBTpng8Y0nLiquDC/FireTVCubeGen2-7.2.0.4-20191004.tar.bz2">FireTVCubeGen2-7.2.0.4-20191004.tar.bz2</a> from Amazon's <a href="https://www.amazon.com/gp/help/customer/display.html?nodeId=201452680">FireTV source code</a> page.

### Installation
Running the compilation script requires installing aarch64-none-elf & arm-none-eabi compiler Toolchains. Follow Odroid's <a href="https://wiki.odroid.com/odroid-n2/software/building_u-boot">Toolchain installation</a> guide. Add the installed compiler paths to <code>build_uboot_config.sh</code>, and run <code>build_uboot_config.sh</code> to update the script paths.

### Compiling the bootloader
To compile the bootloader run:<br>
```
sudo ./build_uboot.sh platform.tar output_directory_name
```

Platform.tar is the compressed bootloader & kernel source from F<a href="https://fireos-tv-src.s3.amazonaws.com/YbHeBIPhSWxBTpng8Y0nLiquDC/FireTVCubeGen2-7.2.0.4-20191004.tar.bz2">FireTVCubeGen2-7.2.0.4-20191004.tar.bz2</a>. Merge the modified files provided here into your platform.tar to remove fastboot & U-Boot command restrictions from your build.

output_directory_name is the completed bootloader image destination.

The component images that make up the bootloader, including bl33.bin.enc can be found in:<br>
<code>tmp/tmp.randomstring/src/bootable/bootloader/uboot-amlogic/s922x/fip/_tmp/</code> 

### Transplanting the patched U-Boot image into the signed bootloader
To insert the modified U-boot image (bl33.bin.enc) into the signed bootloader image obtained from the Amazon FireTV OTA update, overwriting the orginal U-Boot code.

1) Decrypt the Bl33 portion of the signed bootloader with the AES key found at 0x00010EEC (within Bl30) of the signed bootloader. Note: this will require already having extracted the AES key located in the bootrom needed to decrypt Bl2 & Bl30 
```
sudo openssl enc -aes-256-cbc -nopad -d -K 0000000000000000000000000000000000000000000000000000000000000000 -iv 00000000000000000000000000000000 -in u-boot.bin.signed -out bootloader.img
```
2) The patched Bl33 image (bl33.bin.enc) and unmodified Bl33 within the decrypted bootloader (bootloader.img) are now aligned using the LZ4C magic to identify the beginning of the compression scheme used on U-Boot.
```
IN_OFFSET=`grep --byte-offset --only-matching --text LZ4C bl33.bin.enc | head -1 | cut -d: -f1`
OUT_OFFSET=`grep --byte-offset --only-matching --text LZ4C bootloader.img | head -1 | cut -d: -f1`
```
3) Bl33.bin.enc is merged into bootloader.img, and the old Bl33 is overwritten
```
sudo dd if=bl33.bin.enc of=bootloader.img skip=$IN_OFFSET seek=$OUT_OFFSET bs=1 conv=notrunc
```
4) Lastly, we re-encrypt the the end of bootloader.img using the same AES key from step 1).
```
sudo openssl enc -aes-256-cbc -nopad -e -K 0000000000000000000000000000000000000000000000000000000000000000 -iv 00000000000000000000000000000000 -in bootloader.img -out bootloader.img.enc
```

### Modifications made to build script 
Minor modifications were made to the build script to disable the automatic cleanup following compilation completion. Deletion of the working folder is disabled to to make all the components of the bootloader accessible including bl33.bin.enc (patched U-Boot image).

Disable deletion of the working folder<br>
<<a href="https://github.com/Pro-me3us/Raven_Bootloader_Builder/blob/main/build_uboot.sh">build_uboot.sh,/a>> <br>
commented out line 48 <br>
```trap "rm -rf $WORKSPACE_DIR" EXIT```

Disable deletion of the bootloader component images
<platform/bootable/bootloader/uboot-amlogic/s922x/fip/<a href="https://github.com/Pro-me3us/Raven_Bootloader_Builder/blob/main/platform/bootable/bootloader/uboot-amlogic/s922x/fip/mk_script.sh">mk_script.sh,/a>><br>
commented out lines 140-144 <br>
```
function clean() {
	echo "Clean up"
	cd ${UBOOT_SRC_FOLDER}
	make distclean
	cd ${MAIN_FOLDER}
	rm ${FIP_BUILD_FOLDER} -rf
	rm ${BUILD_FOLDER}/* -rf
	return
}
```
	
### Patching Bl33 to remove Amazon's restrictions on Fastboot and U-Boot commands








 


