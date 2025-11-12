#include "MtCompactHelpers.hpp"

void MtCompactHelpers::NodeInfoBuilder(MCT_NodeInfo* nodeinfo, uint32_t node_id, std::string& short_name, std::string& long_name, uint8_t hw_model) {
    nodeinfo->node_id = node_id;
    if (long_name.empty()) {
        char hex_part[7];
        snprintf(hex_part, sizeof(hex_part), "%06" PRIx32, (node_id & 0xFFFFFF));
        hex_part[sizeof(hex_part) - 1] = '\0';
        std::string generated_name = "Meshtastic-" + std::string(hex_part);
        long_name = generated_name;
    }
    if (short_name.empty()) {
        char hex_part[5];
        snprintf(hex_part, sizeof(hex_part), "%04" PRIx32, node_id & 0xFFFF);
        hex_part[sizeof(hex_part) - 1] = '\0';
        short_name = std::string(hex_part);
    }

    strncpy(nodeinfo->short_name, short_name.c_str(), sizeof(nodeinfo->short_name) - 1);
    nodeinfo->short_name[sizeof(nodeinfo->short_name) - 1] = '\0';
    strncpy(nodeinfo->long_name, long_name.c_str(), sizeof(nodeinfo->long_name) - 1);
    nodeinfo->long_name[sizeof(nodeinfo->long_name) - 1] = '\0';
    nodeinfo->hw_model = (uint8_t)meshtastic_HardwareModel_DIY_V1;
    snprintf(nodeinfo->id, sizeof(nodeinfo->id), "!%08" PRIx32, node_id);
    nodeinfo->id[sizeof(nodeinfo->id) - 1] = '0';
    nodeinfo->role = 0;
    for (int i = 0; i < 6; ++i) {
        nodeinfo->macaddr[i] = (node_id >> (8 * (5 - i))) & 0xFF;
    }
    memset(nodeinfo->public_key, 0, sizeof(nodeinfo->public_key));
    nodeinfo->public_key_size = 0;  // Set to 0 if no public key is available
    nodeinfo->hw_model = hw_model;  // Set hardware model
}

void MtCompactHelpers::PositionBuilder(MCT_Position& position, float latitude, float longitude, int32_t altitude, uint32_t speed, uint32_t sats_in_view) {
    position.latitude_i = static_cast<int32_t>(latitude * 10e6);
    position.longitude_i = static_cast<int32_t>(longitude * 10e6);
    position.altitude = altitude;
    position.ground_speed = speed;
    position.sats_in_view = sats_in_view;
    position.location_source = 0;
}

void MtCompactHelpers::TelemetryDeviceBuilder(MCT_Telemetry_Device& telemetry, uint32_t uptime_seconds, float voltage, float battery_level, float channel_utilization) {
    telemetry.uptime_seconds = uptime_seconds;
    telemetry.voltage = voltage;
    telemetry.battery_level = battery_level;
    telemetry.channel_utilization = channel_utilization;
    telemetry.has_uptime_seconds = (uptime_seconds != 0);
    telemetry.has_voltage = (voltage >= 0.0f);
    telemetry.has_battery_level = (battery_level >= 0.0f);
    telemetry.has_channel_utilization = (channel_utilization >= 0.0f);
}

void MtCompactHelpers::TelemetryEnvironmentBuilder(MCT_Telemetry_Environment& telemetry, float temperature, float humidity, float pressure, float lux) {
    telemetry.temperature = temperature;
    telemetry.humidity = humidity;
    telemetry.pressure = pressure;
    telemetry.lux = lux;
    telemetry.has_temperature = (temperature > -10000.0f);
    telemetry.has_humidity = (humidity >= 0.0f);
    telemetry.has_pressure = (pressure >= 0.0f);
    telemetry.has_lux = (lux >= 0.0f);
}

void MtCompactHelpers::WaypointBuilder(MCT_Waypoint& waypoint, uint32_t id, float latitude, float longitude, std::string name, std::string description, uint32_t expire, uint32_t icon) {
    waypoint.latitude_i = static_cast<int32_t>(latitude * 10e6);
    waypoint.longitude_i = static_cast<int32_t>(longitude * 10e6);
    strncpy(waypoint.name, name.c_str(), sizeof(waypoint.name) - 1);
    waypoint.name[sizeof(waypoint.name) - 1] = '\0';
    strncpy(waypoint.description, description.c_str(), sizeof(waypoint.description) - 1);
    waypoint.description[sizeof(waypoint.description) - 1] = '\0';
    waypoint.icon = icon;
    waypoint.expire = expire;
    waypoint.id = id;
    waypoint.has_latitude_i = waypoint.latitude_i != 0;
    waypoint.has_longitude_i = waypoint.longitude_i != 0;
}

void MtCompactHelpers::GeneratePrivateKey(uint8_t* private_key, uint8_t& key_size, uint8_t* public_key) {
    CryptRNG.begin("MeshCompactRNG");
    auto noise = random();
    CryptRNG.stir((uint8_t*)&noise, sizeof(noise));
    Curve25519::dh1(public_key, private_key);
    key_size = 32;
}

void MtCompactHelpers::RegenerateOrGeneratePrivateKey(uint8_t* private_key, uint8_t& key_size, uint8_t* public_key) {
    if (!Curve25519::eval(public_key, private_key, 0)) {
        ESP_LOGI("MtCompactHelpers", "Regenerating private key as existing one is invalid");
        GeneratePrivateKey(private_key, key_size, public_key);
        return;
    }
    key_size = 32;
}
