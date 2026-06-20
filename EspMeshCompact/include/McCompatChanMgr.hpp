#pragma once
#include "McCompactStructs.hpp"

// todo add decode function, and encode function, and search by the channel hash (1st byte of secret)
class McCompatChanMgr {
   public:
    McCompatChanMgr();
    ~McCompatChanMgr();

    bool addChannel(const std::string& name, const uint8_t* secret, size_t size);
    bool addChannel(const std::string& name, const std::string& secret);
    MCC_ChannelEntry* getChannelByName(const std::string& name);
    MCC_ChannelEntry* getChannelByIndex(size_t index);
    MCC_ChannelEntry* getChannelByHashAndData(uint8_t* payload, size_t payload_len, uint8_t* decoded, size_t& out_decoded_len);  // returns null on failure
    size_t getChannelCount();

   private:
    std::vector<MCC_ChannelEntry> channels;
};