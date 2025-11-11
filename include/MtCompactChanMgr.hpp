#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include "MtCompactStructs.hpp"

class MtCompatChanMgr {
   public:
    void addChannel(std::string& name, uint8_t secret[32]) {
        MTC_ChannelEntry entry(name, secret);
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