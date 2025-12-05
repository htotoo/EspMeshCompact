#pragma once

#include <stdint.h>
#include <string>
#include "mbedtls/sha256.h"
#include <cstring>

#define END_OF_MODE_TABLE             \
    {                                 \
        Module::MODE_END_OF_TABLE, {} \
    }

class CompactHelpers {
   public:
    static size_t keyFromString(std::string key, uint8_t* out);
    static uint8_t hexNibble(char c);

    static void sha256(uint8_t* hash, size_t hash_len, const uint8_t* msg, int msg_len);
    static void sha256(uint8_t* hash, size_t hash_len, const uint8_t* frag1, int frag1_len, const uint8_t* frag2, int frag2_len);

    static uint8_t xorHash(const uint8_t* data, size_t len);
};