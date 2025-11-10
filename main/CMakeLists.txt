
file(GLOB_RECURSE MT_SRCS  "../meshtastic/*.cpp")
file(GLOB_RECURSE MY_SRCS "../src/*.cpp")
file(GLOB_RECURSE ED25519_SRCS "../lib/ed25519/*.c0")
file(GLOB_RECURSE CRYPTO_SRCS "../lib/Crypto/*.cpp")
idf_component_register(SRCS "main.cpp"  ${ED25519_SRCS}  ${MY_SRCS} ${MT_SRCS} ${CRYPTO_SRCS} "../src/pb_common.c" "../src/pb_decode.c" "../src/pb_encode.c"
                    INCLUDE_DIRS "." "../" "../lib/ed25519" "../lib/Crypto" "../include" "../meshtastic" REQUIRES driver esp_timer mbedtls radiolib nvs_flash )
