#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include "MtCompactStructs.hpp"

class MtCompatChanMgr {
   public:
    void addDefaultChannels();
    void addDefaultEncryption(std::string name);

    void addChannel(std::string name, uint8_t* secret, size_t secret_len = 32) {
        MTC_ChannelEntry entry(name, secret, secret_len);
        for (const auto& ch : channels) {
            if (ch == entry) {
                return;  // Channel already exists
            }
        }
        channels.push_back(entry);
    }

    MTC_ChannelEntry* getChannelByName(const std::string& name) {
        for (auto& ch : channels) {
            if (ch.name == name) {
                return &ch;
            }
        }
        return nullptr;
    }

    std::vector<MTC_ChannelEntry> channels;
};