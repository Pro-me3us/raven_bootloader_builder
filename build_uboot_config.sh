################################################################################
#
#  build_uboot_config.sh
#
#  Copyright (c) 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
#
################################################################################

UBOOT_SUBPATH="bootable/bootloader/uboot-amlogic/s922x"
export CROSS_COMPILE_STANDALONE=yes

# Expected image files are seperated with ":"
UBOOT_IMAGES="fip/_tmp/u-boot.bin"

################################################################################
# NOTE: You must fill in the following with the path to a copy of an
#       gcc-linaro-aarch64-none-elf-4.8 (aarch64-none-elf compiler) and
#       CodeSourcery g++ lite (arm-none-eabi compiler)
################################################################################
export PATH="$PATH:/opt/toolchains/gcc-linaro-aarch64-none-elf-4.8-2013.11_linux/bin" //edit with path to your build
export PATH="$PATH:/opt/toolchains/gcc-linaro-arm-none-eabi-4.8-2013.11_linux/bin"    //edit with path to your build

