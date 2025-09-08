cmake_minimum_required(VERSION 3.28.1)
project(fwfile C)

set(CMAKE_C_STANDARD 11)

add_executable(fwfile
        fwfile.c
        fw.h
        utils.h
        utils.c
)


add_executable(fw_verification_test
        fw_verification_test.c
        fw.h
)

add_executable(fw_verification_bootloader
        fw_verification_bootloader.c
        fw_verification.h
        fw.h
)

add_executable(keygen
        keygen.c
        utils.h
        utils.c
)


set(TINYCRYPT_SOURCES
        tinycrypt/lib/source/sha256.c
        tinycrypt/lib/source/ecc_dsa.c
        tinycrypt/lib/source/ecc.c
        tinycrypt/lib/source/ecc_dh.c
        tinycrypt/lib/source/utils.c
        tinycrypt/lib/source/ecc_platform_specific.c
)

# Include the TinyCrypt headers
include_directories(tinycrypt/lib/include)

# Add your executable or library here, linking the required TinyCrypt sources

# If building as a library, use:
 add_library(tinycrypt_lib ${TINYCRYPT_SOURCES})
target_link_libraries(mcu tinycrypt_lib)
# Find the OpenSSL package
find_package(OpenSSL REQUIRED)
target_link_libraries(keygen PRIVATE tinycrypt_lib)
target_link_libraries(fwfile PRIVATE tinycrypt_lib)

