#include "McCompactStructs.hpp"
#include "ed_25519.h"
#include "esp_random.h"
#include <ctime>

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

size_t MCC_MyNodeInfo::generate_payload(McPacket_t& packet) {
    size_t pos = packet.length;
    // Copy public key
    std::memcpy(&packet.payload[pos], pubkey, 32);
    pos += 32;
    // Copy timestamp
    timestamp = (uint32_t)time(nullptr);  // set it to now
    std::memcpy(&packet.payload[pos], &timestamp, 4);
    pos += 4;
    // signature, Ed25519 signature of public key, timestamp, and app data
    memset(&packet.payload[pos], 0, 64);  // Clear signature area for now
    pos += 64;

    if (has_location) {
        flags |= (uint8_t)MCC_NODEINFO_FLAGS::HAS_LOCATION;
    } else {
        flags &= ~(uint8_t)MCC_NODEINFO_FLAGS::HAS_LOCATION;
    }
    if (!name.empty()) {
        flags |= (uint8_t)MCC_NODEINFO_FLAGS::HAS_NAME;
    } else {
        flags &= ~(uint8_t)MCC_NODEINFO_FLAGS::HAS_NAME;
    }
    if ((flags & 0xf) == 0) {
        flags |= (uint8_t)MCC_NODEINFO_FLAGS::IS_CHAT_NODE;  // set it as default to chat node if no other type is set
    }
    // Copy flags
    packet.payload[pos++] = flags;
    // Copy optional fields
    if (has_location) {
        std::memcpy(&packet.payload[pos], &latitude_i, 4);
        pos += 4;
        std::memcpy(&packet.payload[pos], &longitude_i, 4);
        pos += 4;
    }
    if (!name.empty()) {
        std::memcpy(&packet.payload[pos], name.c_str(), name.size());
        pos += name.size();
    }

    packet.length = pos;
    return pos;
}