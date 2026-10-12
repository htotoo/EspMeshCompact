#include "CompactHelpers.hpp"
#include "esp_log.h"
#include "psa/crypto.h"

namespace {
constexpr char TAG[] = "CompactHelpers";

bool sha256Fragments(const uint8_t* frag1, size_t frag1_len, const uint8_t* frag2, size_t frag2_len, uint8_t digest[32]) {
    psa_status_t status = psa_crypto_init();
    if (status != PSA_SUCCESS) {
        ESP_LOGE(TAG, "PSA crypto initialization failed: %ld", static_cast<long>(status));
        return false;
    }

    psa_hash_operation_t operation = PSA_HASH_OPERATION_INIT;
    status = psa_hash_setup(&operation, PSA_ALG_SHA_256);
    if (status == PSA_SUCCESS && frag1_len > 0) {
        status = psa_hash_update(&operation, frag1, frag1_len);
    }
    if (status == PSA_SUCCESS && frag2_len > 0) {
        status = psa_hash_update(&operation, frag2, frag2_len);
    }

    size_t digest_len = 0;
    if (status == PSA_SUCCESS) {
        status = psa_hash_finish(&operation, digest, 32, &digest_len);
        if (status != PSA_SUCCESS) {
            psa_hash_abort(&operation);
        }
    } else {
        psa_hash_abort(&operation);
    }
    if (status != PSA_SUCCESS || digest_len != 32) {
        if (status == PSA_SUCCESS) status = PSA_ERROR_GENERIC_ERROR;
        ESP_LOGE(TAG, "PSA SHA-256 failed: %ld", static_cast<long>(status));
        return false;
    }
    return true;
}

void copySha256(uint8_t* hash, size_t hash_len, const uint8_t* frag1, size_t frag1_len, const uint8_t* frag2, size_t frag2_len) {
    uint8_t full_hash[32];
    if (hash == nullptr || hash_len > sizeof(full_hash) ||
        (frag1_len > 0 && frag1 == nullptr) || (frag2_len > 0 && frag2 == nullptr)) {
        ESP_LOGE(TAG, "Invalid SHA-256 input or output length");
        return;
    }
    if (sha256Fragments(frag1, frag1_len, frag2, frag2_len, full_hash)) {
        memcpy(hash, full_hash, hash_len);
    }
}
}

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
    if (msg_len < 0) {
        ESP_LOGE(TAG, "Invalid SHA-256 message length");
        return;
    }
    copySha256(hash, hash_len, msg, static_cast<size_t>(msg_len), nullptr, 0);
};

void CompactHelpers::sha256(uint8_t* hash, size_t hash_len, const uint8_t* frag1, int frag1_len, const uint8_t* frag2, int frag2_len) {
    if (frag1_len < 0 || frag2_len < 0) {
        ESP_LOGE(TAG, "Invalid SHA-256 fragment length");
        return;
    }
    copySha256(hash, hash_len, frag1, static_cast<size_t>(frag1_len), frag2, static_cast<size_t>(frag2_len));
};

uint8_t CompactHelpers::xorHash(const uint8_t* data, size_t len) {
    uint8_t code = 0;
    for (size_t i = 0; i < len; i++)
        code ^= data[i];
    return code;
}
