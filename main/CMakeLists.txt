
file(GLOB_RECURSE MT_SRCS  "../EspMeshCompact/meshtastic/*.cpp")
file(GLOB_RECURSE MY_SRCS "../EspMeshCompact/src/*.cpp")
file(GLOB_RECURSE ED25519_SRCS "../EspMeshCompact/lib/ed25519/*.c")
file(GLOB_RECURSE CRYPTO_SRCS "../EspMeshCompact/lib/Crypto/*.cpp")
idf_component_register(SRCS "main.cpp"  ${ED25519_SRCS}  ${MY_SRCS} ${MT_SRCS} ${CRYPTO_SRCS} "../EspMeshCompact/src/pb_common.c" "../EspMeshCompact/src/pb_decode.c" "../EspMeshCompact/src/pb_encode.c"
                    INCLUDE_DIRS "." "../" "../EspMeshCompact/lib/ed25519" "../EspMeshCompact/" "../EspMeshCompact/lib/Crypto" "../EspMeshCompact/include" "../EspMeshCompact/meshtastic" REQUIRES driver esp_timer mbedtls radiolib nvs_flash )
