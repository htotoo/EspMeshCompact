#pragma once
#include "McCompactStructs.hpp"
#include <string>
#include <inttypes.h>
#include "esp_log.h"

/**
 * @brief Simple static helpers for building message objects easily.
 *
 */
class McCompactHelpers {
   public:
    static void NodeInfoBuilder(MCC_MyNodeInfo* nodeinfo, std::string& name, int32_t latitude_i = 0, int32_t longitude_i = 0, MCC_NODEINFO_FLAGS flags = MCC_NODEINFO_FLAGS::IS_CHAT_NODE, const uint8_t* priv_key = nullptr);
    static void GenerateRandomPath(std::vector<uint32_t>& path, size_t length = 10, uint8_t byte_per_entry = 1);
};
