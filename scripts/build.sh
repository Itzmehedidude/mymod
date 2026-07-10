#!/bin/bash

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Configuration
MODULE_NAME="mymod"
API_LEVEL=24
ARCH="${1:-arm64-v8a}"

echo -e "${GREEN}Building ${MODULE_NAME} for ${ARCH}${NC}"

# Detect NDK
if [ -z "$ANDROID_NDK_HOME" ]; then
    # Check common NDK locations
    if [ -d "/opt/android-ndk" ]; then
        export ANDROID_NDK_HOME="/opt/android-ndk"
    elif [ -d "$HOME/Android/Sdk/ndk" ]; then
        export ANDROID_NDK_HOME=$(ls -d $HOME/Android/Sdk/ndk/* 2>/dev/null | sort -V | tail -n1 || echo "")
    elif [ -d "$ANDROID_HOME/ndk" ]; then
        export ANDROID_NDK_HOME=$(ls -d $ANDROID_HOME/ndk/* 2>/dev/null | sort -V | tail -n1 || echo "")
    elif [ -d "${GITHUB_WORKSPACE}/android-ndk-r26c" ]; then
        export ANDROID_NDK_HOME="${GITHUB_WORKSPACE}/android-ndk-r26c"
    else
        echo -e "${RED}Error: ANDROID_NDK_HOME not set and NDK not found${NC}"
        echo "Please set ANDROID_NDK_HOME environment variable"
        exit 1
    fi
fi

# Verify NDK exists
if [ ! -d "$ANDROID_NDK_HOME" ]; then
    echo -e "${RED}Error: NDK directory not found: $ANDROID_NDK_HOME${NC}"
    exit 1
fi

echo "NDK: $ANDROID_NDK_HOME"

# Set Zygisk headers
if [ -z "$ZYGISK_INCLUDE" ]; then
    # Check common locations
    if [ -d "../ZygiskNext/include" ]; then
        export ZYGISK_INCLUDE=$(pwd)/../ZygiskNext/include
    elif [ -d "ZygiskNext/include" ]; then
        export ZYGISK_INCLUDE=$(pwd)/ZygiskNext/include
    elif [ -d "${GITHUB_WORKSPACE}/ZygiskNext/include" ]; then
        export ZYGISK_INCLUDE="${GITHUB_WORKSPACE}/ZygiskNext/include"
    elif [ -d "/usr/include/zygisk" ]; then
        export ZYGISK_INCLUDE="/usr/include/zygisk"
    else
        echo -e "${YELLOW}Warning: ZYGISK_INCLUDE not set${NC}"
        # Try to fetch
        if [ ! -d "ZygiskNext" ]; then
            git clone --depth 1 https://github.com/Dr-TSNG/ZygiskNext.git
        fi
        export ZYGISK_INCLUDE=$(pwd)/ZygiskNext/include
    fi
fi

echo "Zygisk: $ZYGISK_INCLUDE"

# Verify Zygisk headers
if [ ! -f "$ZYGISK_INCLUDE/zygisk.h" ]; then
    echo -e "${RED}Error: zygisk.h not found in $ZYGISK_INCLUDE${NC}"
    exit 1
fi

# Setup compiler
TOOLCHAIN="${ANDROID_NDK_HOME}/toolchains/llvm/prebuilt/linux-x86_64"

if [ ! -d "$TOOLCHAIN" ]; then
    echo -e "${RED}Error: Toolchain not found: $TOOLCHAIN${NC}"
    exit 1
fi

CLANG="${TOOLCHAIN}/bin"

# Determine compiler
case $ARCH in
    arm64-v8a)
        CLANGXX="${CLANG}/aarch64-linux-android${API_LEVEL}-clang++"
        ;;
    armeabi-v7a)
        CLANGXX="${CLANG}/armv7a-linux-androideabi${API_LEVEL}-clang++"
        ;;
    x86_64)
        CLANGXX="${CLANG}/x86_64-linux-android${API_LEVEL}-clang++"
        ;;
    x86)
        CLANGXX="${CLANG}/i686-linux-android${API_LEVEL}-clang++"
        ;;
    *)
        echo -e "${RED}Unsupported architecture: $ARCH${NC}"
        exit 1
        ;;
esac

# Verify compiler exists
if [ ! -f "$CLANGXX" ]; then
    echo -e "${RED}Error: Compiler not found: $CLANGXX${NC}"
    exit 1
fi

# Create output directory
mkdir -p module/zygisk/${ARCH}
mkdir -p build/${ARCH}

# Compile flags
CFLAGS="-std=c++17 -fPIC -shared"
CFLAGS="${CFLAGS} -fvisibility=hidden -fno-rtti -fno-exceptions"
CFLAGS="${CFLAGS} -I${ZYGISK_INCLUDE}"
CFLAGS="${CFLAGS} -I${TOOLCHAIN}/sysroot/usr/include"
CFLAGS="${CFLAGS} -D__ANDROID_API__=${API_LEVEL}"
CFLAGS="${CFLAGS} -O2 -Wall -Wextra"
CFLAGS="${CFLAGS} -DLOG_TAG=\"${MODULE_NAME}\""

LDFLAGS="-Wl,-z,max-page-size=0x1000 -llog"
LDFLAGS="${LDFLAGS} -Wl,--exclude-libs,ALL"
LDFLAGS="${LDFLAGS} -Wl,--no-undefined"

# Source file
SOURCE_FILE="src/mymod.cpp"
if [ ! -f "$SOURCE_FILE" ]; then
    echo -e "${RED}Error: Source file not found: $SOURCE_FILE${NC}"
    exit 1
fi

# Build
echo -e "${YELLOW}Compiling...${NC}"
echo "Compiler: $CLANGXX"
echo "Source: $SOURCE_FILE"
echo "Output: module/zygisk/${ARCH}/mymod.so"

${CLANGXX} ${CFLAGS} ${LDFLAGS} ${SOURCE_FILE} -o module/zygisk/${ARCH}/mymod.so

if [ $? -eq 0 ] && [ -f "module/zygisk/${ARCH}/mymod.so" ]; then
    echo -e "${GREEN}✓ Build successful!${NC}"
    echo -e "${GREEN}Output: module/zygisk/${ARCH}/mymod.so${NC}"
    ls -lh module/zygisk/${ARCH}/mymod.so
    
    # Verify ELF
    echo -e "${YELLOW}Verifying ELF...${NC}"
    file module/zygisk/${ARCH}/mymod.so || true
    readelf -h module/zygisk/${ARCH}/mymod.so 2>/dev/null | grep "Machine" || true
else
    echo -e "${RED}✗ Build failed!${NC}"
    exit 1
fi

echo -e "${GREEN}Build completed for ${ARCH}${NC}"
