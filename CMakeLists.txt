cmake_minimum_required(VERSION 3.10)
project(mymod)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Zygisk headers
if(NOT DEFINED ZYGISK_INCLUDE)
    set(ZYGISK_INCLUDE "/usr/include/zygisk" CACHE PATH "Zygisk include directory")
endif()

# Add source
add_library(mymod SHARED src/mymod.cpp)

# Include directories
target_include_directories(mymod PRIVATE ${ZYGISK_INCLUDE})

# Compiler options
target_compile_options(mymod PRIVATE
    -fPIC
    -fvisibility=hidden
    -fno-rtti
    -fno-exceptions
    -Wall
    -Wextra
    -O2
    -DLOG_TAG=\"mymod\"
)

# Linker options
target_link_options(mymod PRIVATE
    -Wl,-z,max-page-size=0x1000
    -Wl,--exclude-libs,ALL
)

# Libraries
target_link_libraries(mymod log)