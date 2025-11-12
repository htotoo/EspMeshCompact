#pragma once
#include "MtCompactStructs.hpp"
#include <string>
#include <inttypes.h>
#include "esp_log.h"
#include "AES.h"
#include <RNG.h>
#include <Crypto.h>
#include <Curve25519.h>

#define CryptRNG RNG

/**
 * @brief Simple static helpers for building message objects easily.
 *
 */
class MtCompactHelpers {
   public:
    static void NodeInfoBuilder(MCT_NodeInfo* nodeinfo, uint32_t node_id, std::string& short_name, std::string& long_name, uint8_t hw_model);
    static void PositionBuilder(MCT_Position& position, float latitude, float longitude, int32_t altitude = 0, uint32_t speed = 0, uint32_t sats_in_view = 0);
    static void TelemetryDeviceBuilder(MCT_Telemetry_Device& telemetry, uint32_t uptime_seconds = 0, float voltage = 0.0f, float battery_level = -1.0f, float channel_utilization = -1.0f);
    static void TelemetryEnvironmentBuilder(MCT_Telemetry_Environment& telemetry, float temperature = -10000.0f, float humidity = -1.0f, float pressure = -1.0f, float lux = -1.0f);
    static void WaypointBuilder(MCT_Waypoint& waypoint, uint32_t id, float latitude, float longitude, std::string name, std::string description, uint32_t expire = 1, uint32_t icon = 0);
    static void GeneratePrivateKey(uint8_t* private_key, uint8_t& key_size, uint8_t* public_key);
    static void GeneratePrivateKey(MCT_MyNodeInfo& my_nodeinfo) {
        GeneratePrivateKey(my_nodeinfo.private_key, my_nodeinfo.public_key_size, my_nodeinfo.public_key);
    }
    static void RegenerateOrGeneratePrivateKey(uint8_t* private_key, uint8_t& key_size, uint8_t* public_key);
    static void RegenerateOrGeneratePrivateKey(MCT_MyNodeInfo& my_nodeinfo) {
        RegenerateOrGeneratePrivateKey(my_nodeinfo.private_key, my_nodeinfo.public_key_size, my_nodeinfo.public_key);
    }
};
