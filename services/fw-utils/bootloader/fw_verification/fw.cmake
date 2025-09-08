
function(get_fw_sources lib_dir SOURCES INCLUDES)
    set(utils_path ${lib_dir}/../..)
    set(TINYCRYPT_SOURCES
            ${utils_path}/tinycrypt/lib/source/sha256.c
            ${utils_path}/tinycrypt/lib/source/ecc_dsa.c
            ${utils_path}/tinycrypt/lib/source/ecc.c
            ${utils_path}/tinycrypt/lib/source/ecc_dh.c
            ${utils_path}/tinycrypt/lib/source/utils.c
            ${utils_path}/tinycrypt/lib/source/ecc_platform_specific.c
    )

    set(${INCLUDES}
            ${utils_path}/tinycrypt/lib/include
            ${utils_path}/tinycrypt/lib/include/tinycrypt
            ${lib_dir}/.
            PARENT_SCOPE )
    set(${SOURCES}
            ${lib_dir}/fw_verification.c
            ${lib_dir}/fw_verification.h
            ${lib_dir}/fw.h
            ${TINYCRYPT_SOURCES}
            PARENT_SCOPE
            )


endfunction()