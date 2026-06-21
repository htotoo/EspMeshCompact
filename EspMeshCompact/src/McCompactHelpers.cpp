#include "McCompactHelpers.hpp"

void McCompactHelpers::NodeInfoBuilder(MCC_MyNodeInfo* nodeinfo, std::string& name, int32_t latitude_i, int32_t longitude_i, MCC_NODEINFO_FLAGS flags, const uint8_t* priv_key) {
    if (!nodeinfo) return;
    nodeinfo->name = name;
    nodeinfo->latitude_i = latitude_i;
    nodeinfo->longitude_i = longitude_i;
    nodeinfo->has_location = (latitude_i != 0 || longitude_i != 0);
    nodeinfo->flags = static_cast<uint8_t>(flags);
    if (priv_key) {
        nodeinfo->setPrivateKey(priv_key);
    } else {
        nodeinfo->generateKeyPair();
    }
}

void McCompactHelpers::GenerateRandomPath(std::vector<uint32_t>& path, size_t length, uint8_t byte_per_entry) {
    path.clear();
    for (size_t i = 0; i < length; ++i) {
        uint32_t entry = 0;
        for (uint8_t j = 0; j < byte_per_entry; ++j) {
            entry |= (random() & 0xFF) << (j * 8);
        }
        path.push_back(entry);
    }
}