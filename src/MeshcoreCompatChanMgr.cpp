#include "MeshcoreCompatChanMgr.hpp"
MeshcoreCompatChanMgr::MeshcoreCompatChanMgr() {}
MeshcoreCompatChanMgr::~MeshcoreCompatChanMgr() {}
bool MeshcoreCompatChanMgr::addChannel(const std::string& name, const uint8_t* secret) {
    MCC_ChannelEntry entry;
    entry.name = name;
    std::memcpy(entry.secret, secret, 32);
    channels.push_back(entry);
    return true;
}

MCC_ChannelEntry* MeshcoreCompatChanMgr::getChannelByName(const std::string& name) {
    for (auto& channel : channels) {
        if (channel.name == name) {
            return &channel;
        }
    }
    return nullptr;
}

MCC_ChannelEntry* MeshcoreCompatChanMgr::getChannelByIndex(size_t index) {
    if (index < channels.size()) {
        return &channels[index];
    }
    return nullptr;
}

size_t MeshcoreCompatChanMgr::getChannelCount() {
    return channels.size();
}
MCC_ChannelEntry* MeshcoreCompatChanMgr::getChannelByHashAndData(uint8_t* payload, size_t payload_len, uint8_t* decoded, size_t& out_decoded_len) {
    // todo
    return nullptr;
}