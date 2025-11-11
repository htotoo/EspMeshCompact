#include "CompactHelpers.hpp"
#include "mbedtls/sha256.h"

size_t CompactHelpers::keyFromString(std::string key, uint8_t* out) {
    const char* cstr = key.c_str();
    size_t len = key.length();
    for (size_t i = 0, j = 0; i < len; i += 2, ++j) {
        uint8_t high = hexNibble(cstr[i]);
        uint8_t low = hexNibble(cstr[i + 1]);
        out[j] = (high << 4) | low;
    }
    return len / 2;
}
uint8_t CompactHelpers::hexNibble(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    } else if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    } else {
        return 0;  // Invalid character
    }
}

void CompactHelpers::sha256(uint8_t* hash, size_t hash_len, const uint8_t* msg, int msg_len) {
    mbedtls_sha256_context sha256;
    mbedtls_sha256_init(&sha256);
    mbedtls_sha256_starts(&sha256, 0);
    int ret = mbedtls_sha256_update(&sha256, msg, msg_len);
    uint8_t full_hash[32];
    ret = mbedtls_sha256_finish(&sha256, full_hash);
    (void)ret;
    mbedtls_sha256_free(&sha256);
    memcpy(hash, full_hash, hash_len);
};

void CompactHelpers::sha256(uint8_t* hash, size_t hash_len, const uint8_t* frag1, int frag1_len, const uint8_t* frag2, int frag2_len) {
    mbedtls_sha256_context sha256;
    uint8_t full_hash[32];
    mbedtls_sha256_init(&sha256);
    mbedtls_sha256_starts(&sha256, 0);
    int ret = mbedtls_sha256_update(&sha256, frag1, frag1_len);
    ret = mbedtls_sha256_update(&sha256, frag2, frag2_len);
    ret = mbedtls_sha256_finish(&sha256, full_hash);
    (void)ret;
    memcpy(hash, full_hash, hash_len);
    mbedtls_sha256_free(&sha256);
};

uint8_t CompactHelpers::xorHash(const uint8_t* data, size_t len) {
    uint8_t code = 0;
    for (size_t i = 0; i < len; i++)
        code ^= data[i];
    return code;
}
