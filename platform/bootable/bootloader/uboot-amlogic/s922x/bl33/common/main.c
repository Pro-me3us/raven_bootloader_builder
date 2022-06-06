/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/* #define	DEBUG	*/

#include <common.h>
#include <autoboot.h>
#include <cli.h>
#include <version.h>
#if defined(UFBL_FEATURE_FASTBOOT_LOCKDOWN)
#include "amzn_fastboot_lockdown.h"
#else
#error "UFBL_FEATURE_FASTBOOT_LOCKDOWN is required"
#endif
#ifdef CONFIG_IDME
#include <idme.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

/*
 * Board-specific Platform code can reimplement show_boot_progress () if needed
 */
__weak void show_boot_progress(int val) {}

static void modem_init(void)
{
#ifdef CONFIG_MODEM_SUPPORT
	debug("DEBUG: main_loop:   gd->do_mdm_init=%lu\n", gd->do_mdm_init);
	if (gd->do_mdm_init) {
		char *str = getenv("mdm_cmd");

		setenv("preboot", str);  /* set or delete definition */
		mdm_init(); /* wait for modem connection */
	}
#endif  /* CONFIG_MODEM_SUPPORT */
}

static void run_preboot_environment_command(void)
{
#ifdef CONFIG_PREBOOT
	char *p;

	p = getenv("preboot");
	if (p != NULL) {
# ifdef CONFIG_AUTOBOOT_KEYED
		int prev = disable_ctrlc(1);	/* disable Control C checking */
# endif

		run_command_list(p, -1, 0);

# ifdef CONFIG_AUTOBOOT_KEYED
		disable_ctrlc(prev);	/* restore Control C checking */
# endif
	}
#endif /* CONFIG_PREBOOT */
}

/* We come here after U-Boot is initialised and ready to process commands */
void main_loop(void)
{
	const char *s;

	bootstage_mark_name(BOOTSTAGE_ID_MAIN_LOOP, "main_loop");

#ifndef CONFIG_SYS_GENERIC_BOARD
	puts("Warning: Your board does not use generic board. Please read\n");
	puts("doc/README.generic-board and take action. Boards not\n");
	puts("upgraded by the late 2014 may break or be removed.\n");
#endif

	modem_init();
#ifdef CONFIG_VERSION_VARIABLE
	setenv("ver", version_string);  /* set version variable */
#endif /* CONFIG_VERSION_VARIABLE */

	cli_init();

#ifdef CONFIG_IDME
#define DEV_FLAGS_SELINUX_FORCE_ENFORCING 32
#define DEV_FLAGS_SELINUX_FORCE_PERMISSIVE 64
	char dev_flags_str[19] = { 0 };
	unsigned long long dev_flags = 0;

	if(0 != idme_get_var_external("dev_flags", dev_flags_str, 16)) {
		printf("Error getting dev_flags value\n");
	} else {
		dev_flags = simple_strtoul(dev_flags_str, NULL, 16);
		if ((dev_flags & DEV_FLAGS_SELINUX_FORCE_ENFORCING) == DEV_FLAGS_SELINUX_FORCE_ENFORCING) {
			printf("Force selinux enforcing mode\n");
			setenv("EnableSelinux", "enforcing");  

		} else if ((dev_flags & DEV_FLAGS_SELINUX_FORCE_PERMISSIVE) == DEV_FLAGS_SELINUX_FORCE_PERMISSIVE) {
			printf("Force selinux permissive mode\n");
			setenv("EnableSelinux", "permissive");
		}
	}


#endif /* CONFIG_IDME */
	run_preboot_environment_command();

#if defined(CONFIG_UPDATE_TFTP)

	update_tftp(0UL);
#endif /* CONFIG_UPDATE_TFTP */

	s = bootdelay_process();
	if (cli_process_fdt(&s))
		cli_secure_boot_cmd(s);
		

#if defined(CONFIG_AML_UBOOT_AUTO_TEST)
	//stick 0 and stick 1 will be used to check the boot process of uboot
	//stick 0 is the start counter (0xC8834400 + 0x7C<<2) = 0xc88345f0
	//stick 1 is the end counter   (0xC8834400 + 0x7D<<2) = 0xc88345f4
	if (*((volatile unsigned int*)(0xc88345f0)))
	{
		printf("\n\naml log : TE = %d\n",*((volatile unsigned int*)0xc1109988));
		*((volatile unsigned int*)(0xc88345f4)) += 1; //stick 1
		printf("\n\naml log : Boot success %d times @ %d\n",*((volatile unsigned int*)(0xc88345f4)),
			*((volatile unsigned int*)(0xc88345f0))); //stick 0 set in bl2_main.c
		int ndelay = 10;
		int nabort = 0;
		while (ndelay)
		{
			udelay(5);
			if (tstc())
			switch (getc())
			{
			//case 0x20: /* Space */
			case 0x0d: /* Enter */
				nabort = 1;
				break;
			}
			ndelay -= 1;
		}
		if (!nabort)
			run_command("reset",0);

	}
#endif //#if defined(CONFIG_AML_UBOOT_AUTO_TEST)
	run_command("fastboot", 0);  //change "fastboot" to "update" for Amlogic burn mode
	run_preboot_environment_command();

	autoboot_command(s);    //to boot to uboot cmdline, comment out this line as well as lines 147 and 148
#if defined(UFBL_FEATURE_FASTBOOT_LOCKDOWN)
	/* We are hitting interactive prompt, begin commands lock down */
	amzn_block_commands();

#endif
	cli_loop();
}
