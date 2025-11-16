#include <stdio.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include "McCompact.hpp"

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

    mesh.RadioInit(RadioType::SX1262, radio_pins, lora_config_mc);
    mesh.chan_mgr.addChannel("TTest", "38c42a7bf8c329d8e7759ac8b1f83d96");
    mesh.setOnNodeInfo([](const MCC_Nodeinfo& info) {
        printf("Node Info received: NodeID=%u, Name=%s\n",
               *((uint8_t*)&info.pubkey[28]),
               info.name.c_str());
    });
    mesh.setOnGroupMsg([](const MCC_ChannelEntry& channel, const std::string& msg) {
        printf("Group Msg received on channel %s: %s\n", channel.name.c_str(), msg.c_str());
    });

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}