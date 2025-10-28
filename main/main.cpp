#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include "MeshcoreCompact.hpp"

Radio_PINS radio_pins = {9, 11, 10, 8, 14, 12, 13};  // Default radio pins for Heltec WSL V3.
LoraConfig lora_config = {
    .frequency = 869.618,   // config
    .bandwidth = 62.5,      // config
    .spreading_factor = 8,  // config
    .coding_rate = 8,       // config
    .sync_word = 0x12,
    .preamble_length = 16,
    .output_power = 22,  // config
    .tcxo_voltage = 1.8,
    .use_regulator_ldo = false,
};  //

MeshcoreCompact mesh;

void onRaw(const uint8_t* data, size_t len) {
    printf("Received packet of length %zu: ", len);
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
};

void onNodeInfo(const MCC_Nodeinfo& info) {
    printf("Received NodeInfo: timestamp=%lu, flags=0x%02x, name: %s\n", info.timestamp, info.flags, info.name.c_str());
};

void onGroupMsg(const MCC_ChannelEntry& channel, const std::string& msg) {
    printf("Received Group Message on channel %s: %s\n", channel.name.c_str(), msg.c_str());
};

extern "C" void app_main(void) {
    mesh.RadioInit(RadioType::SX1262, radio_pins, lora_config);
    mesh.setOnRaw(onRaw);
    mesh.setOnNodeInfo(onNodeInfo);
    mesh.setOnGroupMsg(onGroupMsg);
    mesh.chan_mgr.addChannel("Public", "8b3387e9c5cdea6ac9e5edbaa115cd72");
    mesh.chan_mgr.addChannel("TTest", "38c42a7bf8c329d8e7759ac8b1f83d96");
    printf("Hello, world!\n");
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}