#!/bin/bash

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Configuration
MODULE_NAME="mymod"
API_LEVEL=24
ARCH="${1:-arm64-v8a}"

# Detect NDK
if [ -z "$ANDROID_NDK_HOME" ]; then
    if [ -d "/opt/android-ndk" ]; then
        export ANDROID_NDK_HOME="/opt/android-ndk"
    elif [ -d "$HOME/Android/Sdk/ndk" ]; then
        export ANDROID_NDK_HOME=$(ls -d $HOME/Android/Sdk/ndk/* | sort -V | tail -n1)
    else
        echo -e "${RED}Error: ANDROID_NDK_HOME not set${NC}"
        exit 1
    fi
fi

# Set Zygisk headers
if [ -z "$ZYGISK_INCLUDE" ]; then
    if [ -d "../ZygiskNext/include" ]; then
        export ZYGISK_INCLUDE=$(pwd)/../ZygiskNext/include
    elif [ -d "ZygiskNext/include" ]; then
        export ZYGISK_INCLUDE=$(pwd)/ZygiskNext/include
    elif [ -d "/usr/include/zygisk" ]; then
        export ZYGISK_INCLUDE="/usr/include/zygisk"
    else
        echo -e "${YELLOW}Warning: ZYGISK_INCLUDE not set, using default${NC}"
        # Try to fetch if not exists
        if [ ! -d "ZygiskNext" ]; then
            git clone --depth 1 https://github.com/Dr-TSNG/ZygiskNext.git
        fi
        export ZYGISK_INCLUDE=$(pwd)/ZygiskNext/include
    fi
fi

echo -e "${GREEN}Building ${MODULE_NAME} for ${ARCH}${NC}"
echo "NDK: $ANDROID_NDK_HOME"
echo "Zygisk: $ZYGISK_INCLUDE"

# Compiler setup
TOOLCHAIN="${ANDROID_NDK_HOME}/toolchains/llvm/prebuilt/linux-x86_64"
CLANG="${TOOLCHAIN}/bin"

case $ARCH in
    arm64-v8a)
        CLANG++="${CLANG}/aarch64-linux-android${API_LEVEL}-clang++"
        ;;
    armeabi-v7a)
        CLANG++="${CLANG}/armv7a-linux-androideabi${API_LEVEL}-clang++"
        ;;
    x86_64)
        CLANG++="${CLANG}/x86_64-linux-android${API_LEVEL}-clang++"
        ;;
    x86)
        CLANG++="${CLANG}/i686-linux-android${API_LEVEL}-clang++"
        ;;
    *)
        echo -e "${RED}Unsupported architecture: $ARCH${NC}"
        exit 1
        ;;
esac

# Create output directory
mkdir -p module/zygisk/${ARCH}
mkdir -p build/${ARCH}

# Compile flags
CFLAGS="-std=c++17 -fPIC -shared -fvisibility=hidden"
CFLAGS="${CFLAGS} -fno-rtti -fno-exceptions"
CFLAGS="${CFLAGS} -I${ZYGISK_INCLUDE}"
CFLAGS="${CFLAGS} -I${TOOLCHAIN}/sysroot/usr/include"
CFLAGS="${CFLAGS} -D__ANDROID_API__=${API_LEVEL}"
CFLAGS="${CFLAGS} -O2 -Wall -Wextra"
CFLAGS="${CFLAGS} -DLOG_TAG=\"${MODULE_NAME}\""

LDFLAGS="-Wl,-z,max-page-size=0x1000 -llog -Wl,--exclude-libs,ALL"

# Build
echo -e "${YELLOW}Compiling...${NC}"
${CLANG++} ${CFLAGS} ${LDFLAGS} src/mymod.cpp -o module/zygisk/${ARCH}/mymod.so

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Build successful!${NC}"
    echo -e "${GREEN}Output: module/zygisk/${ARCH}/mymod.so${NC}"
    ls -lh module/zygisk/${ARCH}/mymod.so
    
    # Verify ELF
    echo -e "${YELLOW}Verifying ELF...${NC}"
    file module/zygisk/${ARCH}/mymod.so
else
    echo -e "${RED}✗ Build failed!${NC}"
    exit 1
fi