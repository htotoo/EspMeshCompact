#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include "MtCompact.hpp"

Radio_PINS radio_pins = {9, 11, 10, 8, 14, 12, 13};  // Default radio pins for Heltec WSL V3.
LoraConfig lora_config_mc = {
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

LoraConfig lora_config_mt = {
    .frequency = 869.525,   // config
    .bandwidth = 250,       // config
    .spreading_factor = 9,  // config
    .coding_rate = 5,       // config
    .sync_word = 0x12,
    .preamble_length = 16,
    .output_power = 22,  // config
    .tcxo_voltage = 1.8,
    .use_regulator_ldo = false,
};  //

MtCompact mesh;

extern "C" void app_main(void) {
    mesh.RadioInit(RadioType::SX1262, radio_pins, lora_config_mt);
    mesh.setOkToMqtt(true);
    std::string sn = "Info";
    std::string ln = "Hungarian Info Node";
    MtCompactHelpers::NodeInfoBuilder(mesh.getMyNodeInfo(), 0xabbababa, sn, ln, 1);
    MtCompactHelpers::PositionBuilder(mesh.my_position, 47.497913, 19.040236, 120);
    mesh.setSendEnabled(true);
    mesh.setSendHopLimit(7);
    MtCompactHelpers::RegenerateOrGeneratePrivateKey(*mesh.getMyNodeInfo());

    printf("Hello, world!\n");
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}