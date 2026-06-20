
file(GLOB_RECURSE MT_SRCS  "../meshcompact/meshtastic/*.cpp")
file(GLOB_RECURSE MY_SRCS "../meshcompact/src/*.cpp")
file(GLOB_RECURSE ED25519_SRCS "../meshcompact/lib/ed25519/*.c")
file(GLOB_RECURSE CRYPTO_SRCS "../meshcompact/lib/Crypto/*.cpp")
idf_component_register(SRCS "main.cpp"  ${ED25519_SRCS}  ${MY_SRCS} ${MT_SRCS} ${CRYPTO_SRCS} "../meshcompact/src/pb_common.c" "../meshcompact/src/pb_decode.c" "../meshcompact/src/pb_encode.c"
                    INCLUDE_DIRS "." "../" "../meshcompact/lib/ed25519" "../meshcompact/" "../meshcompact/lib/Crypto" "../meshcompact/include" "../meshcompact/meshtastic" REQUIRES driver esp_timer mbedtls radiolib nvs_flash )
