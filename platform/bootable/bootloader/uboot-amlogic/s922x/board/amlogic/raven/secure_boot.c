/*
 * secure_boot.c
 *
 * Copyright 2011-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 */

#include <asm/io.h>
#include <common.h>
#include "amzn_secure_boot.h"
#if defined(UFBL_FEATURE_UNLOCK)
#include <amzn_unlock.h>
#include <u-boot/sha256.h>
#endif
#include <asm/arch/secure_apb.h>

#if defined(UFBL_FEATURE_ONETIME_UNLOCK)
#include <amzn_onetime_unlock.h>
#include "onetime_unlock_key.h"
#endif

const char *amzn_target_device_name(void)
{
	return "raven";
}

bool secure_boot_enabled(void)
{
	const unsigned long cfg10 = readl(AO_SEC_SD_CFG10);
	return ( (cfg10 & (0x1<< 4)) ? true : false );
	/* 4th bit indicates secure boot status */
}

int amzn_target_device_type(void)
{
	/* Is anti-rollback enabled? */
	if (query_efuse_status("ARB") == 1)
		return AMZN_ENGINEERING_DEVICE;
	else
		return AMZN_ENGINEERING_DEVICE;
}

#if defined(UFBL_FEATURE_UNLOCK)

#define CHIPID_UPPER (6)
#define CHIPID_LOWER (7)
#define CHIPID_BUF_SIZE (16)
#define HASH_BUF_SIZE (32)

int amzn_get_unlock_code(unsigned char *code, unsigned int *len)
{
	sha256_context ctx;
	uint8_t buff[CHIPID_BUF_SIZE] = {0};
	uint8_t hash[HASH_BUF_SIZE] = {0};

	if (!code || !len || *len < (16 + 1))
		return -1;

	if (get_chip_id(&buff[0], sizeof(buff)))
		return -1;
	/**
	* To sync with Amazon serial number from kernel's /proc/cpuinfo,
	* the unlock_code is low 64 bit of sha256(SoC Chipid 128bits).
	*/
	sha256_starts(&ctx);
	sha256_update(&ctx, &buff[0], sizeof(buff));
	sha256_finish(&ctx, &hash[0]);
	u32 *hashcode = (u32 *) &hash[0];
	snprintf(code, CHIPID_BUF_SIZE, "%08x%08x",be32_to_cpu(hashcode[CHIPID_UPPER]),
			be32_to_cpu(hashcode[CHIPID_LOWER]));

	*len = 16;
	return 0;
}

const unsigned char *amzn_get_unlock_key(unsigned int *key_len)
{
	//unlock_raven.pub
	static const unsigned char unlock_key[] =
	"\x30\x82\x01\x22\x30\x0d\x06\x09\x2a\x86\x48\x86\xf7\x0d\x01\x01"
	"\x01\x05\x00\x03\x82\x01\x0f\x00\x30\x82\x01\x0a\x02\x82\x01\x01"
	"\x00\xdf\xdf\x56\xbc\x4f\xb0\x52\x8f\xee\x3f\x3c\x5f\xd8\xd7\x1c"
	"\xab\x50\x2e\xed\x89\x50\x09\x99\x82\xf5\xce\xd9\xde\x5a\xe2\x29"
	"\xa0\x2a\x62\x54\xda\xf6\x21\x51\xf1\x4a\xbe\x66\x37\xb8\xe3\xdc"
	"\xc9\x6e\x0d\x60\x73\x4a\x1a\xbb\x07\xe3\x75\x38\xe4\x85\xc2\x66"
	"\x4c\x0b\x88\x12\x16\xa0\x11\x83\xf2\x8d\xba\x2b\xa1\x2a\x01\x75"
	"\x82\x5c\x40\x86\xa4\xd9\x67\x22\xc0\x29\x06\xb9\xb4\xdd\x14\x14"
	"\x2b\xea\x73\xf1\x88\x2d\xe9\xf8\xc0\xc2\x64\x17\xb1\xc7\x66\xfc"
	"\x8e\x36\xa2\x15\x3d\x50\x86\x7d\xda\x07\x7f\xd0\x49\x01\x76\xf3"
	"\xd9\x41\x4e\x0b\x0d\x18\xb2\x90\x24\xad\x25\xc9\x8a\x66\x4a\xa7"
	"\xf9\x3f\x9d\x43\xcb\xc1\x51\xb0\x31\xbc\x0f\xef\x9b\x4c\xc4\x5e"
	"\x00\x7b\x49\xa1\x65\x58\x5d\x0b\xc5\x58\x9a\x98\x5c\xbb\x7c\xb4"
	"\x04\xf1\x4e\x33\xac\x4e\x50\x7a\xa6\x18\x4c\x00\x47\x5c\xe2\xa8"
	"\x74\xd5\x44\x4c\x4a\x75\x3e\x67\x2d\xb9\x9f\x97\xd5\x82\xa8\xde"
	"\xc4\x40\x10\x1b\x82\xf5\xac\x8e\xae\x13\xb3\x8a\x52\xd6\x03\x52"
	"\xd2\x1b\x11\xe0\x27\xe1\x32\x28\x41\x31\xcd\xc6\xd8\x66\xb4\xc1"
	"\x70\x3d\x09\xd3\xe1\xd1\x27\x33\xbf\x43\xd2\x6f\x0e\x64\x4c\x5f"
	"\x67\x02\x03\x01\x00\x01"
	;

	const int unlock_key_size = sizeof(unlock_key);
	if (!key_len)
		return NULL;

	*key_len = unlock_key_size;

	return unlock_key;
}

#if defined(UFBL_FEATURE_ONETIME_UNLOCK)
int amzn_get_one_tu_code(unsigned char *code, unsigned int *len)
{
	static unsigned char code_generated = 0;
	static unsigned char one_tu_code[ONETIME_UNLOCK_CODE_LEN + 1] = {0};

	if (!code || !len || *len < ONETIME_UNLOCK_CODE_LEN)
		return -1;

	if (!code_generated) {
/**
 * Different SoC/Product may have different scheme to add entropy into PRNG.
 * For raven, take unlock code plus get_timer (8 bytes) as entropy
 */
#define ENTROPY_LEN (UNLOCK_CODE_LEN + 8)
		static unsigned char entropy[ENTROPY_LEN] = {0};
		unsigned int unlock_code_len = UNLOCK_CODE_LEN;
		if (amzn_get_unlock_code(entropy, &unlock_code_len)) {
			return -1;
		}
		sprintf(&entropy[unlock_code_len], "%08x", get_timer(0));
/**
 * amzn_get_onetime_random_number will return a binary string which is not readable.
 * The binary string cannot be returned via fastboot so using base64 encode it.
 */
// compute how many random bytes do we need so that the converted size is the target length
#define RANDOM_BYTES_SIZE (ONETIME_UNLOCK_CODE_LEN + 3) / 4 * 3
		uint8_t random_bytes[RANDOM_BYTES_SIZE] = {0};
		unsigned int out_len = sizeof(one_tu_code);

		if (amzn_get_onetime_random_number(entropy, strlen(entropy),
						random_bytes, sizeof(random_bytes)))
			return -1;

		if (amzn_onetime_unlock_b64_encode(random_bytes, sizeof(random_bytes),
						one_tu_code, &out_len)) {
			return -1;
		}
		code_generated = 1;
	}
	memcpy(code, one_tu_code, ONETIME_UNLOCK_CODE_LEN);
	*len = ONETIME_UNLOCK_CODE_LEN;
	return 0;
}

int amzn_get_onetime_unlock_root_pubkey(unsigned char **key, unsigned int *key_len)
{
	static const unsigned char onetime_unlock_key[] = ONETIME_UNLOCK_KEY;
	const int onetime_unlock_key_size = sizeof(onetime_unlock_key);

	if (!key || !key_len)
		return -1;

	*key_len = onetime_unlock_key_size;
	*key = onetime_unlock_key;
	return 0;
}
#endif //UFBL_FEATURE_ONETIME_UNLOCK
#endif //UFBL_FEATURE_UNLOCK
