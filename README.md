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

Platform.tar is the compressed bootloader & kernel source from <a href="https://fireos-tv-src.s3.amazonaws.com/YbHeBIPhSWxBTpng8Y0nLiquDC/FireTVCubeGen2-7.2.0.4-20191004.tar.bz2">FireTVCubeGen2-7.2.0.4-20191004.tar.bz2</a>. Merge the modified files provided here into your platform.tar to remove fastboot & U-Boot command restrictions from your build.

output_directory_name is the completed bootloader image destination.

The component images that make up the bootloader, including <code>bl33.bin.enc</code> can be found in:<br>
<code>tmp/tmp.randomstring/src/bootable/bootloader/uboot-amlogic/s922x/fip/_tmp/</code> 

### Transplanting the patched U-Boot image into the signed bootloader
To insert the modified U-boot image (<code>bl33.bin.enc</code>) into the signed bootloader image obtained from the Amazon FireTV OTA update, overwriting the orginal U-Boot code.

1) Decrypt the Bl33 portion of the signed bootloader with the AES key found at 0x00010EEC (within Bl30) of the signed bootloader. Note: this will require already having extracted the AES key located in the bootrom needed to decrypt Bl2 & Bl30 
```
sudo openssl enc -aes-256-cbc -nopad -d -K 0000000000000000000000000000000000000000000000000000000000000000 -iv 00000000000000000000000000000000 -in u-boot.bin.signed -out bootloader.img
```
2) The patched Bl33 image (<code>bl33.bin.enc</code>) and unmodified Bl33 within the decrypted bootloader (<code>bootloader.img</code>) are now aligned using the LZ4C magic to identify the beginning of the compression scheme used on U-Boot.
```
IN_OFFSET=`grep --byte-offset --only-matching --text LZ4C bl33.bin.enc | head -1 | cut -d: -f1`
OUT_OFFSET=`grep --byte-offset --only-matching --text LZ4C bootloader.img | head -1 | cut -d: -f1`
```
3) <code>Bl33.bin.enc</code> is merged into bootloader.img, and the old Bl33 is overwritten
```
sudo dd if=bl33.bin.enc of=bootloader.img skip=$IN_OFFSET seek=$OUT_OFFSET bs=1 conv=notrunc
```
4) Lastly, we re-encrypt the the end of <code>bootloader.img</code> using the same AES key from step 1).
```
sudo openssl enc -aes-256-cbc -nopad -e -K 0000000000000000000000000000000000000000000000000000000000000000 -iv 00000000000000000000000000000000 -in bootloader.img -out bootloader.img.enc
```



## File modifications made
### Build script 
Minor modifications were made to the build script to disable the automatic cleanup following compilation completion. Deletion of the working folder is disabled to to make all the components of the bootloader accessible including <code>bl33.bin.enc</code> (patched U-Boot image).

[<a href="https://github.com/Pro-me3us/Raven_Bootloader_Builder/blob/main/build_uboot.sh">build_uboot.sh</a>]<br>
Disable deletion of the working folder<br>
commented out line 48 <br>
```
#trap "rm -rf $WORKSPACE_DIR" EXIT
```

[<a href="https://github.com/Pro-me3us/Raven_Bootloader_Builder/blob/main/platform/bootable/bootloader/uboot-amlogic/s922x/fip/mk_script.sh">mk_script.sh</a>]<br>
Disable deletion of the bootloader component images<br>
commented out lines 140-144 <br>
```
function clean() {
	echo "Clean up"
#	cd ${UBOOT_SRC_FOLDER}
#	make distclean
#	cd ${MAIN_FOLDER}
#	rm ${FIP_BUILD_FOLDER} -rf
#	rm ${BUILD_FOLDER}/* -rf
	return
}
```
	
### Removing Fastboot and U-Boot command restrictions
**[amzn_lockdown.c]** Original<br>
Removing Amazon's U-Boot commandline restrictions<br>
```
bool amzn_is_command_blocked(const char *cmd)
{
	int i = 0, found = 0;

	/* Are we in lock down? */
	if (lockdown_commands == false)
		return false;

	/* Is this an engineering device? */
	if (amzn_target_device_type() == AMZN_ENGINEERING_DEVICE)
		return false;

	/* Are we un-locked? */
	if (amzn_target_is_unlocked())
		return false;

	if (amzn_target_is_onetime_unlocked())
		return false;

	/* If command is on the white-list, allow */
	for (i = 0; i < ARRAY_SIZE(whitelisted_commands); i++)
		if (strcmp(whitelisted_commands[i], cmd) == 0)
			found = 1;

	/* Not on the white-list? Block */
	if (!found)
		return true;

	return false;
}
```

[<a href="https://github.com/Pro-me3us/Raven_Bootloader_Builder/blob/main/platform/bootable/bootloader/uboot-amlogic/s922x/bl33/common/amzn_lockdown.c">amzn_lockdown.c</a>] Modified<br>
Edited down function and moved up to line 22<br>

```
bool amzn_is_command_blocked(const char *cmd)
{
	return false;
}
```
=====================================================<br>

**[amzn_fastboot_lockdown.c]** Original<br>
Removing Amazon's Fastboot command restrictions<br>
```
__attribute__((weak)) int is_locked_production_device() {
#if defined(UFBL_FEATURE_SECURE_BOOT)
	return (AMZN_PRODUCTION_DEVICE == amzn_target_device_type()) && (1 != g_boot_arg->unlocked);
#else
	return 0;
#endif
}

#else /* UFBL_PROJ_ABC */

__attribute__((weak)) int is_locked_production_device() {
#if defined(UFBL_FEATURE_SECURE_BOOT) && defined(UFBL_FEATURE_UNLOCK)
	return (AMZN_PRODUCTION_DEVICE == amzn_target_device_type()
                        && (!amzn_target_is_unlocked())
#if defined(UFBL_FEATURE_TEMP_UNLOCK)
                        && (!amzn_target_is_temp_unlocked())
#endif
#if defined(UFBL_FEATURE_ONETIME_UNLOCK)
                        && (!amzn_target_is_onetime_unlocked())
#endif
			);
#else
	return 0;
#endif
}

#endif /* UFBL_PROJ_ABC */
```

[<a href="https://github.com/Pro-me3us/Raven_Bootloader_Builder/blob/main/platform/bootable/bootloader/ufbl-features/features/fastboot_lockdown/amzn_fastboot_lockdown.c">amzn_fastboot_lockdown.c</a>] 1st Patch<br>
```
__attribute__((weak)) int is_locked_production_device() {
    return 0;
}
```

**[amzn_fastboot_lockdown.c]** Original<br>
```
	for (i = 0; i < sizeof(blacklist) / sizeof(blacklist[0]); ++i) {
		if (memcmp(buffer, blacklist[i], strlen(blacklist[i])) == 0) {
			return 1;
		}
	}

	amzn_extends_fastboot_blacklist(&list, &length);
	if (list != NULL && length > 0) {
		for (i = 0; i < length; ++i) {
			if (memcmp(buffer, list[i], strlen(list[i])) == 0) {
				return 1;
			}
		}
	}

	return 0;
}
```

[<a href="https://github.com/Pro-me3us/Raven_Bootloader_Builder/blob/main/platform/bootable/bootloader/ufbl-features/features/fastboot_lockdown/amzn_fastboot_lockdown.c">amzn_fastboot_lockdown.c</a>] 2nd Patch<br>
```
	for (i = 0; i < sizeof(blacklist) / sizeof(blacklist[0]); ++i) {
		if (memcmp(buffer, blacklist[i], strlen(blacklist[i])) == 0) {
			return 0;
		}
	}

	amzn_extends_fastboot_blacklist(&list, &length);
	if (list != NULL && length > 0) {
		for (i = 0; i < length; ++i) {
			if (memcmp(buffer, list[i], strlen(list[i])) == 0) {
				return 0;
			}
		}
	}

	return 0;
}
```
=====================================================<br>

**[image_verify.c]** Original<br>
Remove the fastboot flash image verification check<br>
``` 
int
amzn_image_verify(const void *image,
		  unsigned char *signature,
		  unsigned int image_size, meta_data_handler handler)
{
	int auth = 0;
	char *digest = NULL;

	if (!(digest = amzn_plat_alloc(SHA256_DIGEST_LENGTH))) {
		dprintf(CRITICAL, "ERROR: Unable to allocate image hash\n");
		goto cleanup;
	}

	memset(digest, 0, SHA256_DIGEST_LENGTH);

	/*
	 * Calculate hash of image for comparison
	 */
	amzn_target_sha256(image, image_size, digest);

	if (amzn_verify_image(AMZN_PRODUCTION_CERT, digest,
					signature, handler)) {
		if (amzn_target_device_type() == AMZN_PRODUCTION_DEVICE) {
			dprintf(ALWAYS,
				"Image FAILED AUTHENTICATION on PRODUCTION device\n");
			/* Failed verification */
			goto cleanup;
		} else {
		        dprintf(ALWAYS,
				"Authentication failed on engineering device with production certificate\n");
		}

		if (amzn_target_device_type() != AMZN_ENGINEERING_DEVICE) {
			dprintf(ALWAYS,
				"%s: Unknown device type!\n", UFBL_STR(__FUNCTION__));
			goto cleanup;
		}

		/* Engineering device */
		if (amzn_verify_image(AMZN_ENGINEERING_CERT, digest,
					signature, handler)) {
			dprintf(ALWAYS,
				"Image FAILED AUTHENTICATION on ENGINEERING device\n");
			goto cleanup;
		}
	} else {
		dprintf(ALWAYS,
			"Image AUTHENTICATED with PRODUCTION certificate\n");
	}

	auth = 1;

cleanup:
	if (digest)
		amzn_plat_free(digest);

	return auth;
}
```

[<a href="https://github.com/Pro-me3us/Raven_Bootloader_Builder/blob/main/platform/bootable/bootloader/ufbl-features/features/secure_boot/image_verify.c">image_verify.c</a>] Patched<br>
```
int
amzn_image_verify(const void *image,
          unsigned char *signature,
          unsigned int image_size, meta_data_handler handler)
{
    return 1;
}
```
=====================================================<br>

**[secure_boot.c]** Original<br>
Designate the Cube as an engineering device.  This is a redundancy that should cover any restrictions that may have been missed.
```
int amzn_target_device_type(void)
{
	/* Is anti-rollback enabled? */
	if (query_efuse_status("ARB") == 1)
		return AMZN_PRODUCTION_DEVICE;
	else
		return AMZN_ENGINEERING_DEVICE;
}
```

[<a href="https://github.com/Pro-me3us/Raven_Bootloader_Builder/blob/main/platform/bootable/bootloader/uboot-amlogic/s922x/board/amlogic/raven/secure_boot.c">secure_boot.c</a>] Patched<br>
```
int amzn_target_device_type(void)
{
	/* Is anti-rollback enabled? */
	if (query_efuse_status("ARB") == 1)
		return AMZN_ENGINEERING_DEVICE;
	else
		return AMZN_ENGINEERING_DEVICE;
}
```
