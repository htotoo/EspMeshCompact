#include <stdio.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include "McCompact.hpp"
#include "McCompactHelpers.hpp"

Radio_PINS radio_pins = {9, 11, 10, 8, 14, 12, 13};  // Default radio pins for Heltec WSL V3. // https://github.com/meshtastic/firmware/blob/81828c6244daede254cf759a0f2bd939b2e7dd65/variants/heltec_wsl_v3/variant.h

LoraConfig lora_config_mc = {
    /*.frequency = */ 869.618,   // config
    /*.bandwidth = */ 62.5,      // config
    /*.spreading_factor = */ 8,  // config
    /*.coding_rate = */ 8,       // config
    /*.sync_word = */ 0x12,
    /*.preamble_length = */ 16,
    /*.output_power = */ 22,  // config
    /*.tcxo_voltage = */ 1.8,
    /*.use_regulator_ldo = */ false,
};  //

McCompact mesh;

extern "C" void app_main(void) {
    // nvsinit.must be done!
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    mesh.setDebugMode(true);  // Enable debug logging
    mesh.RadioInit(RadioType::SX1262, radio_pins, lora_config_mc);
    mesh.chan_mgr.addChannel("ttest", "7a568182fd29c410c53675107ea67ed3");
    mesh.chan_mgr.addChannel("Public", "8b3387e9c5cdea6ac9e5edbaa115cd72");
    mesh.chan_mgr.addChannel("hungary", "d2ad7e4009b727fb4ee5c1ff51694e5e");
    mesh.chan_mgr.addChannel("ping", "3cae16fd067ba9c32a98be22e9b98525");
    mesh.chan_mgr.addChannel("info", "ce51a275a0a0507c43d1651d78292320");

    mesh.setOnRaw([](const uint8_t* data, size_t len) {
        printf("Raw data received: ");
        for (size_t i = 0; i < len; ++i) {
            printf("%02X ", data[i]);
        }
        printf("\n");
    });

    mesh.setOnNodeInfo([](const MCC_Nodeinfo& info) {
        printf("Node Info received: NodeID=%u, Name=%s, Time: %lu\n",
               *((uint8_t*)&info.pubkey[28]),
               info.name.c_str(),
               info.timestamp);
        if (time(nullptr) < 3000000) {
            // set esp time from nodeinfo timestamp
            struct timeval tv;
            tv.tv_sec = info.timestamp;
            tv.tv_usec = 0;
            settimeofday(&tv, nullptr);
        }
    });
    mesh.setOnGroupMsg([](const MCC_ChannelEntry& channel, const std::string& msg, uint32_t timestamp, uint8_t flags) {
        printf("Group Msg (flags=0x%02x) received at %lu on channel %s: %s\n", flags, timestamp, channel.name.c_str(), msg.c_str());
        if (time(nullptr) < 3000000) {
            // set esp time from nodeinfo timestamp
            struct timeval tv;
            tv.tv_sec = timestamp;
            tv.tv_usec = 0;
            settimeofday(&tv, nullptr);
        }
    });
    std::string name = "TestNode";
    McCompactHelpers::NodeInfoBuilder(mesh.getMyNodeInfo(), name, 47.4979, 19.0402, MCC_NODEINFO_FLAGS::IS_CHAT_NODE);

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(30000));
        MCC_Path path_data(1);
        McCompactHelpers::GenerateRandomPath(path_data, 10 + rand() % 10);
        std::string message = "ping";
        MCC_ChannelEntry* channel = mesh.chan_mgr.getChannelByName("ping");
        if (channel != nullptr) {
            if (time(nullptr) > 1782161287) {
                mesh.sendGroupMsg(*channel, name, message, path_data);
            }
        }
    }
}