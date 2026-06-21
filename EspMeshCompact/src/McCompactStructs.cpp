#include "McCompactStructs.hpp"
#include "ed_25519.h"
#include "esp_random.h"

bool MCC_MyNodeInfo::generateKeyPair() {
    uint8_t seed[32];
    // Generate a random seed
    for (int i = 0; i < 32; ++i) {
        seed[i] = (uint8_t)esp_random();
    }

    ed25519_create_keypair(pubkey, priv_key, seed);

    return true;
}

void MCC_MyNodeInfo::calcSharedSecret(uint8_t* secret, const uint8_t* other_pub_key) const {
    ed25519_key_exchange(secret, other_pub_key, priv_key);
}

void MCC_MyNodeInfo::setPrivateKey(const uint8_t* priv) {
    std::memcpy(priv_key, priv, sizeof(priv_key));
    ed25519_derive_pub(pubkey, priv_key);
}

void MCC_MyNodeInfo::sign(uint8_t* sig, const uint8_t* message, int msg_len) const {
    ed25519_sign(sig, message, msg_len, pubkey, priv_key);
}