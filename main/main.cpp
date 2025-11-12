#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include "MtCompact.hpp"

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

LoraConfig lora_config_mt = {
    /*.frequency = */ 869.525,   // config
    /*.bandwidth = */ 250,       // config
    /*.spreading_factor = */ 9,  // config
    /*.coding_rate = */ 5,       // config
    /*.sync_word = */ 0x2b,
    /*.preamble_length = */ 16,
    /*.output_power = */ 22,  // config
    /*.tcxo_voltage = */ 1.8,
    /*.use_regulator_ldo = */ false,
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
    mesh.setPrimaryChanByHash(31);
    mesh.setAutoFullNode(false);
    // priv:
    //  f0, 69, 3d, fd, 3e, 2c, 68, cb, b3, cc, 09, e6, db, 6c, e0, 6a, f7, ea, 33, aa, 3c, df, de, af, d3, aa, e5, 0c, 22, ba, 0b, 74,
    // pub:
    // 88, 18, 07, 3e, d4, 38, 6d, e3, b0, 1a, 90, df, 1e, 98, f4, 10, 75, e9, 60, 19, 62, 5c, d4, ff, 96, 56, ab, 1e, a2, 6b, 0a, 4a,
    uint8_t my_p_key[32] = {0xf0, 0x69, 0x3d, 0xfd, 0x3e, 0x2c, 0x68, 0xcb,
                            0xb3, 0xcc, 0x09, 0xe6, 0xdb, 0x6c, 0xe0, 0x6a,
                            0xf7, 0xea, 0x33, 0xaa, 0x3c, 0xdf, 0xde, 0xaf,
                            0xd3, 0xaa, 0xe5, 0x0c, 0x22, 0xba, 0x0b, 0x74};

    memcpy(mesh.getMyNodeInfo()->private_key, my_p_key, 32);
    MtCompactHelpers::RegenerateOrGeneratePrivateKey(*mesh.getMyNodeInfo());
    mesh.loadPrivKey();
    mesh.loadNodeDb();
    mesh.chan_mgr.addDefaultChannels();
    mesh.chan_mgr.addDefaultEncryption("Hungary");
    mesh.sendMyNodeInfo();
    vTaskDelay(pdMS_TO_TICKS(10000));
    // ESP_LOGI("Main", "Sending nodeinfo...");
    mesh.sendMyNodeInfo(0xb29facf4, true);
    vTaskDelay(pdMS_TO_TICKS(20000));
    std::string test_msg = "Hello from EspMeshtasticCompact!";
    ESP_LOGI("Main", "Sending test message");
    // mesh.sendTextMessage(test_msg, 0x0xb29facf4, 0);
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(60000));
        mesh.sendTextMessage(test_msg, 0xb29facf4, 0, MCT_MESSAGE_TYPE_ALERT, 0);
        if (mesh.nodeinfo_db.needsSave(60000)) {
            ESP_LOGI("Main", "Saving Node DB...");
            mesh.saveNodeDb();
        }
    }
}