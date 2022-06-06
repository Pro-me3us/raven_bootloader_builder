/* Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved. */

#include <common.h>

#if defined(UFBL_FEATURE_SECURE_BOOT)
#include "amzn_secure_boot.h"
#else
#error "UFBL_FEATURE_SECURE_BOOT is required"
#endif

#if defined(UFBL_FEATURE_UNLOCK)
#include "amzn_unlock.h"
#else
#error "UFBL_FEATURE_UNLOCK is required"
#endif

/*
 * At U-Boot prompt, we only allow fastboot and reset
 * commands unless the device is unlocked (via fastboot), OR
 * it is an engineering device
 */
bool amzn_is_command_blocked(const char *cmd)
{
	return false;
}

static const char *whitelisted_commands[] = {
	"fastboot",
	"reset",
	"reboot",
	"help"
};

static bool lockdown_commands = false;

void amzn_block_commands(void)
{
	lockdown_commands = true;
}
