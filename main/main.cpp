#include <stdio.h>
#include "nvs_flash.h"
#include "nvs.h"
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
    // nvsinit.must be done!
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

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
    mesh.setDebugMode(true);
    mesh.loadNodeDb();
    mesh.savePrivKey();
    mesh.chan_mgr.addDefaultChannels();
    mesh.chan_mgr.addDefaultEncryption("Hungary");
    mesh.sendMyNodeInfo();
    mesh.setOnMessage([](MCT_Header& header, MCT_TextMessage& message) {
        ESP_LOGI("OnMessage", "Received message from node 0x%08" PRIx32 ": %s", header.srcnode, message.text.c_str());
    });
    vTaskDelay(pdMS_TO_TICKS(10000));
    // ESP_LOGI("Main", "Sending nodeinfo...");
    // mesh.sendMyNodeInfo(0xa0cc18fc, true);
    vTaskDelay(pdMS_TO_TICKS(20000));
    std::string test_msg = "pong 18 hops";

    // mesh.sendTextMessage(test_msg, 0xffffffff, 92, MCT_MESSAGE_TYPE_TEXT, 0, 2257061455, true);
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(30000));
        ESP_LOGI("Main", "Sending test message");
        mesh.sendTextMessage(test_msg, 0x433ad734, 31, MCT_MESSAGE_TYPE_TEXT, 0, 4074877044, true);
        if (mesh.nodeinfo_db.needsSave(60000 * 5)) {
            ESP_LOGI("Main", "Saving Node DB...");
            mesh.saveNodeDb();
            mesh.nodeinfo_db.clearChangedFlag();
        }
    }
}