################################################################################
#
#  build_kernel_config.sh
#
#  Copyright (c) 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
#
################################################################################

KERNEL_SUBPATH="kernel/mediatek/mt8183/4.4"
DEFCONFIG_NAME="maverick_defconfig"
TARGET_ARCH="arm64"
TOOLCHAIN_REPO="https://android.googlesource.com/platform/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9"
TOOLCHAIN_BRANCH="llvm-r383902b"
TOOLCHAIN_NAME="aarch64-linux-android-4.9"
TOOLCHAIN_PREFIX="aarch64-linux-android-"
PARALLEL_EXECUTION="-j24"

# Expected image files are seperated with ":"
KERNEL_IMAGES="arch/arm64/boot/Image:arch/arm64/boot/Image.gz:arch/arm64/boot/Image.gz-dtb"

################################################################################
# NOTE: You must fill in the following with the path to a copy of Clang compiler
#       of version 4691093.
################################################################################
CLANG_COMPILER_PATH=""
