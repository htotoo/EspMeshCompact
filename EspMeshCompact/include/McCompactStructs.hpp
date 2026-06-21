#ifndef MeshcoreCompactStructs_h
#define MeshcoreCompactStructs_h

#include <stdint.h>
#include <string>
#include <vector>
#include "RadioStructs.hpp"
#include <cstring>

typedef struct {
    uint16_t length;
    uint8_t payload[260];
} McPacket_t;

enum class MCC_ROUTE_TYPE {
    ROUTE_TYPE_TRANSPORT_FLOOD = 0,
    ROUTE_TYPE_FLOOD = 1,
    ROUTE_TYPE_DIRECT = 2,
    ROUTE_TYPE_TRANSPORT_DIRECT = 3
};

enum class MCC_PAYLOAD_TYPE : uint8_t {
    PAYLOAD_TYPE_REQ = 0x00,
    PAYLOAD_TYPE_RESPONSE = 0x01,
    PAYLOAD_TYPE_TXT_MSG = 0x02,
    PAYLOAD_TYPE_ACK = 0x03,
    PAYLOAD_TYPE_ADVERT = 0x04,
    PAYLOAD_TYPE_GRP_TXT = 0x05,
    PAYLOAD_TYPE_GRP_DATA = 0x06,
    PAYLOAD_TYPE_ANON_REQ = 0x07,
    PAYLOAD_TYPE_PATH = 0x08,
    PAYLOAD_TYPE_TRACE = 0x09,
    PAYLOAD_TYPE_MULTIPART = 0x0A,
    PAYLOAD_TYPE_CONTROL = 0x0B,
    PAYLOAD_TYPE_RAW_CUSTOM = 0x0F
};

enum class MCC_PAYLOADVER : uint8_t {
    PAYLOAD_V1 = 0x00,  // 1-byte src/dest hashes, 2-byte MAC
    PAYLOAD_V2 = 0x01,  // Future version: 2-byte hashes, 4-byte MAC
    PAYLOAD_V3 = 0x02,  // Future version
    PAYLOAD_V4 = 0x03   // Future version
};

enum class MCC_NODEINFO_FLAGS : uint8_t {
    IS_CHAT_NODE = 0x01,    // advert is for a chat node
    IS_REPEATER = 0x02,     // advert is for a repeater
    IS_ROOM_SERVER = 0x03,  // advert is for a room server
    IS_SENSOR = 0x04,       // advert is for a sensor server
    HAS_LOCATION = 0x10,    // appdata contains lat/long information
    HAS_FEATURE_1 = 0x20,   // Reserved for future use
    HAS_FEATURE_2 = 0x40,   // Reserved for future use
    HAS_NAME = 0x80         // appdata contains a node name
};

class MCC_Header {
   public:
    MCC_ROUTE_TYPE get_route_type() {
        return (MCC_ROUTE_TYPE)(header & 0x03);
    }
    MCC_PAYLOAD_TYPE get_payload_type() {
        return (MCC_PAYLOAD_TYPE)((header >> 2) & 0x0F);
    }
    MCC_PAYLOADVER get_payload_version() {
        return (MCC_PAYLOADVER)((header >> 6) & 0x03);
    }

    uint32_t get_transport_codes() {
        return transport_codes;
    }

    const char* get_route_type_str() {
        switch (get_route_type()) {
            case MCC_ROUTE_TYPE::ROUTE_TYPE_TRANSPORT_FLOOD:
                return "TRANSPORT_FLOOD";
            case MCC_ROUTE_TYPE::ROUTE_TYPE_FLOOD:
                return "FLOOD";
            case MCC_ROUTE_TYPE::ROUTE_TYPE_DIRECT:
                return "DIRECT";
            case MCC_ROUTE_TYPE::ROUTE_TYPE_TRANSPORT_DIRECT:
                return "TRANSPORT_DIRECT";
            default:
                return "UNKNOWN";
        }
    }

    const char* get_payload_type_str() {
        switch (get_payload_type()) {
            case MCC_PAYLOAD_TYPE::PAYLOAD_TYPE_REQ:
                return "REQ";
            case MCC_PAYLOAD_TYPE::PAYLOAD_TYPE_RESPONSE:
                return "RESPONSE";
            case MCC_PAYLOAD_TYPE::PAYLOAD_TYPE_TXT_MSG:
                return "TXT_MSG";
            case MCC_PAYLOAD_TYPE::PAYLOAD_TYPE_ACK:
                return "ACK";
            case MCC_PAYLOAD_TYPE::PAYLOAD_TYPE_ADVERT:
                return "ADVERT";
            case MCC_PAYLOAD_TYPE::PAYLOAD_TYPE_GRP_TXT:
                return "GRP_TXT";
            case MCC_PAYLOAD_TYPE::PAYLOAD_TYPE_GRP_DATA:
                return "GRP_DATA";
            case MCC_PAYLOAD_TYPE::PAYLOAD_TYPE_ANON_REQ:
                return "ANON_REQ";
            case MCC_PAYLOAD_TYPE::PAYLOAD_TYPE_PATH:
                return "PATH";
            case MCC_PAYLOAD_TYPE::PAYLOAD_TYPE_TRACE:
                return "TRACE";
            case MCC_PAYLOAD_TYPE::PAYLOAD_TYPE_MULTIPART:
                return "MULTIPART";
            case MCC_PAYLOAD_TYPE::PAYLOAD_TYPE_CONTROL:
                return "CONTROL";
            case MCC_PAYLOAD_TYPE::PAYLOAD_TYPE_RAW_CUSTOM:
                return "RAW_CUSTOM";
            default:
                return "UNKNOWN";
        }
    }

    /**
     * @brief Parse the MCC header from the given data buffer.
     *
     * @param data Pointer to the data buffer.
     * @param len Length of the data buffer.
     * @return size_t Number of bytes consumed from the buffer, or 0 on failure.
     */

    size_t parse(uint8_t* data, size_t len) {
        if (len < 4) return 0;  // todo better solution, and further checks
        size_t pos = 0;
        header = data[pos++];
        if (get_route_type() == MCC_ROUTE_TYPE::ROUTE_TYPE_TRANSPORT_FLOOD || get_route_type() == MCC_ROUTE_TYPE::ROUTE_TYPE_TRANSPORT_DIRECT) {
            // in this case we have transport code 4 bytes
            transport_codes = *((uint32_t*)&data[pos]);
            pos += 4;
        } else {
            transport_codes = 0;
        }
        uint8_t meta = data[pos++];
        uint8_t path_size = ((meta >> 6) & 0x03) + 1;      // 1, 2, or 3 bytes per hop
        uint8_t path_count = (meta & 0x3F);                // Number of hops
        if (len < pos + path_count * path_size) return 0;  // Not enough data for the path
        path.resize(path_count);
        for (int i = 0; i < path_count; ++i) {
            uint32_t current_hop = 0;
            for (int b = 0; b < path_size; ++b) {
                current_hop |= (uint32_t)data[pos++] << (8 * b);
            }

            path[i] = current_hop;
        }
        header_end_pos = pos;
        return pos;
    }

    size_t parse(McPacket_t* packet) {
        return parse(packet->payload, packet->length);
    }

    size_t generate_header(uint8_t* buffer, size_t buffer_len, uint8_t route_type, uint8_t payload_type, std::vector<uint32_t> path, uint8_t path_bytenum = 1, uint32_t transport_code = 0) {
        if (buffer_len < 4) return 0;
        size_t pos = 0;
        header = (route_type & 0x03) | ((payload_type & 0x0F) << 2) | (((uint8_t)MCC_PAYLOADVER::PAYLOAD_V1 & 0x03) << 6);
        buffer[pos++] = header;
        if (get_route_type() == MCC_ROUTE_TYPE::ROUTE_TYPE_TRANSPORT_FLOOD || get_route_type() == MCC_ROUTE_TYPE::ROUTE_TYPE_TRANSPORT_DIRECT) {
            // in this case we have transport code 4 bytes
            *((uint32_t*)&buffer[pos]) = transport_code;
            pos += 4;
        }
        uint8_t meta = ((path_bytenum - 1) & 0x03) << 6;  // path_size is 1,2,3 -> store as 0,1,2
        meta |= (path.size() & 0x3F);
        buffer[pos++] = meta;
        for (const auto& hop : path) {
            for (int b = 0; b < path_size; ++b) {
                buffer[pos++] = (hop >> (8 * b)) & 0xFF;
            }
        }
        return pos;
    }

    size_t generate_header(McPacket_t* packet, uint8_t route_type, uint8_t payload_type, std::vector<uint32_t> path, uint8_t path_bytenum = 1, uint32_t transport_code = 0) {
        size_t header_len = generate_header(packet->payload, sizeof(packet->payload), route_type, payload_type, path, path_bytenum, transport_code);
        if (header_len > 0) {
            packet->length = header_len;
        }
        return header_len;
    }

    uint8_t header;
    uint32_t transport_codes;    // optional 4 bytes
    std::vector<uint32_t> path;  // it'll store path_len too
    uint8_t path_size = 1;       // 1, 2, or 3 bytes per hop
    size_t header_end_pos = 0;   // position in the buffer where the header ends. 0 = not decoded
};

class MCC_Nodeinfo {
   public:
    bool operator==(const MCC_Nodeinfo& other) const {
        return std::memcmp(pubkey, other.pubkey, sizeof(pubkey)) == 0;
    }
    MCC_Nodeinfo& operator=(const MCC_Nodeinfo& other) {
        if (this != &other) {
            std::memcpy(pubkey, other.pubkey, sizeof(pubkey));
            timestamp = other.timestamp;
            flags = other.flags;
            latitude_i = other.latitude_i;
            longitude_i = other.longitude_i;
            name = other.name;
        }
        return *this;
    }

    size_t parse(const uint8_t* data, size_t start_pos, size_t len) {
        if (len < 36 + start_pos) return 0;  // minimum size
        size_t pos = start_pos;
        memcpy(pubkey, &data[pos], 32);
        pos += 32;
        timestamp = *((uint32_t*)&data[pos]);
        pos += 4;
        // skip signature
        pos += 64;
        flags = data[pos++];
        if (flags & (uint8_t)MCC_NODEINFO_FLAGS::HAS_LOCATION) {
            if (len < pos + 7) return 0;
            latitude_i = *((int32_t*)&data[pos]);
            pos += 4;
            longitude_i = *((int32_t*)&data[pos]);
            pos += 4;
            has_location = true;
        } else {
            has_location = false;
        }
        if (flags & (uint8_t)MCC_NODEINFO_FLAGS::HAS_FEATURE_1) {
            // skip feature 1 data for now
            pos += 2;
        }
        if (flags & (uint8_t)MCC_NODEINFO_FLAGS::HAS_FEATURE_2) {
            // skip feature 2 data for now
            pos += 2;
        }
        if (flags & (uint8_t)MCC_NODEINFO_FLAGS::HAS_NAME) {
            uint8_t name_len = len - pos;
            name = std::string((const char*)&data[pos], name_len);
            pos += name_len;
        }
        return pos;
    }

    bool isChat() const {
        return (flags & (uint8_t)MCC_NODEINFO_FLAGS::IS_CHAT_NODE) != 0;
    }
    bool isRepeater() const {
        return (flags & (uint8_t)MCC_NODEINFO_FLAGS::IS_REPEATER) != 0;
    }
    bool isRoomServer() const {
        return (flags & (uint8_t)MCC_NODEINFO_FLAGS::IS_ROOM_SERVER) != 0;
    }
    bool isSensor() const {
        return (flags & (uint8_t)MCC_NODEINFO_FLAGS::IS_SENSOR) != 0;
    }

    uint8_t pubkey[32];
    uint32_t timestamp;
    uint8_t flags;
    int32_t latitude_i;   // optional
    int32_t longitude_i;  // optional
    std::string name;
    bool has_location = false;
};

class MCC_MyNodeInfo : public MCC_Nodeinfo {
   public:
    MCC_MyNodeInfo() {
        std::memset(priv_key, 0, sizeof(priv_key));
        std::memset(pubkey, 0, sizeof(pubkey));
    }
    void sign(uint8_t* sig, const uint8_t* message, int msg_len) const;
    void calcSharedSecret(uint8_t* secret, const uint8_t* other_pub_key) const;
    void setPrivateKey(const uint8_t* priv);

    bool generateKeyPair();

    uint8_t priv_key[64];
};

class MCC_ChannelEntry {
   public:
    std::string name;
    uint8_t secret[32];
    uint8_t secret_len;
    uint8_t hash[1];
};

#endif  // MeshCoreCompactStructs_h