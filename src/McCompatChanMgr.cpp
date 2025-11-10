#include "esp_log.h"
#include "McCompatChanMgr.hpp"
#include "CompactHelpers.hpp"
#include "McCompact.hpp"

McCompatChanMgr::McCompatChanMgr() {}
McCompatChanMgr::~McCompatChanMgr() {}
bool McCompatChanMgr::addChannel(const std::string& name, const uint8_t* secret, size_t size) {
    MCC_ChannelEntry entry;
    entry.name = name;
    memcpy(entry.secret, secret, size);
    CompactHelpers::sha256(entry.hash, sizeof(entry.hash), secret, size);
    entry.secret_len = size;
    channels.push_back(entry);
    return true;
}

bool McCompatChanMgr::addChannel(const std::string& name, const std::string& secret) {
    uint8_t key[32] = {0};
    auto len = CompactHelpers::keyFromString(secret, key);
    return addChannel(name, key, len);
}

MCC_ChannelEntry* McCompatChanMgr::getChannelByName(const std::string& name) {
    for (auto& channel : channels) {
        if (channel.name == name) {
            return &channel;
        }
    }
    return nullptr;
}

MCC_ChannelEntry* McCompatChanMgr::getChannelByIndex(size_t index) {
    if (index < channels.size()) {
        return &channels[index];
    }
    return nullptr;
}

size_t McCompatChanMgr::getChannelCount() {
    return channels.size();
}

MCC_ChannelEntry* McCompatChanMgr::getChannelByHashAndData(uint8_t* payload, size_t payload_len, uint8_t* decoded, size_t& out_decoded_len) {
    // todo
    for (auto& channel : channels) {
        ESP_LOGI("ChanMgr", "Trying channel %s with hash 0x%02x  :   0x%02x ", channel.name.c_str(), channel.hash[0], payload[0]);
        if (channel.hash[0] == payload[0]) {
            ESP_LOGI("ChanMgr", "Channel %s matched hash", channel.name.c_str());
            auto lenn = McCompact::MACThenDecrypt(channel.secret, decoded, payload + 1, payload_len - 1);
            if (lenn > 0) {  // success!
                // onGroupDataRecv(pkt, pkt->getPayloadType(), channels[j], data, lenn);
                out_decoded_len = lenn;
                ESP_LOGI("ChanMgr", "Decrypted with channel %s", channel.name.c_str());
                return &channel;
            } else {
                ESP_LOGI("ChanMgr", "Failed to decrypt with channel %s, lenn: %d", channel.name.c_str(), lenn);
            }
        }
    }
    out_decoded_len = 0;
    return nullptr;
}