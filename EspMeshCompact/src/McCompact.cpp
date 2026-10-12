#include "McCompact.hpp"
#include "esp_log.h"
#include "psa/crypto.h"

#include "esp_mac.h"
#define TAG "McCompact"

namespace {
psa_status_t importAesKey(const uint8_t* key, size_t key_len, psa_algorithm_t algorithm, psa_key_usage_t usage, psa_key_id_t* key_id) {
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
    psa_set_key_bits(&attributes, key_len * 8);
    psa_set_key_usage_flags(&attributes, usage);
    psa_set_key_algorithm(&attributes, algorithm);
    psa_status_t status = psa_import_key(&attributes, key, key_len, key_id);
    psa_reset_key_attributes(&attributes);
    return status;
}

bool initializePsaCrypto() {
    psa_status_t status = psa_crypto_init();
    if (status != PSA_SUCCESS) {
        ESP_LOGE(TAG, "PSA crypto initialization failed: %ld", static_cast<long>(status));
        return false;
    }
    return true;
}

bool computeHmacSha256(const uint8_t* key, const uint8_t* message, size_t message_len, uint8_t mac[32]) {
    if (key == nullptr || mac == nullptr || (message_len > 0 && message == nullptr) || !initializePsaCrypto()) {
        return false;
    }

    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_type(&attributes, PSA_KEY_TYPE_HMAC);
    psa_set_key_bits(&attributes, CIPHER_KEY_SIZE * 8);
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_MESSAGE);
    psa_set_key_algorithm(&attributes, PSA_ALG_HMAC(PSA_ALG_SHA_256));
    psa_key_id_t key_id;
    psa_status_t status = psa_import_key(&attributes, key, CIPHER_KEY_SIZE, &key_id);
    psa_reset_key_attributes(&attributes);
    if (status != PSA_SUCCESS) {
        ESP_LOGE(TAG, "HMAC key import failed: %ld", static_cast<long>(status));
        return false;
    }

    size_t mac_len = 0;
    status = psa_mac_compute(key_id, PSA_ALG_HMAC(PSA_ALG_SHA_256), message, message_len,
                             mac, 32, &mac_len);
    psa_status_t destroy_status = psa_destroy_key(key_id);
    if (status != PSA_SUCCESS || mac_len != 32) {
        if (status == PSA_SUCCESS) status = PSA_ERROR_GENERIC_ERROR;
        ESP_LOGE(TAG, "HMAC computation failed: %ld", static_cast<long>(status));
        return false;
    }
    if (destroy_status != PSA_SUCCESS) {
        ESP_LOGE(TAG, "HMAC key cleanup failed: %ld", static_cast<long>(destroy_status));
        return false;
    }
    return true;
}
}

volatile bool packetFlag = false;
volatile bool packet_during_send = false;

static_assert(CONFIG_ESP_MAIN_TASK_STACK_SIZE >= 8000, "Main task stack size must be at least 8000 bytes!");

void IRAM_ATTR onPacketReceived() {
    packetFlag = true && !packet_during_send;  // Only set the flag if not currently sending
}

McCompact::McCompact() {
}

McCompact::~McCompact() {
    need_run = false;  // Stop the tasks
    packetFlag = true;
    out_queue.stop_wait();                 // Notify any waiting pop() to unblock immediately
    vTaskDelay(150 / portTICK_PERIOD_MS);  // Give some time for tasks to finish
    if (radio) {
        radio->standby();
        delete radio;  // Delete the radio instance
        radio = nullptr;
    }
    if (hal) {
        hal->term();
        delete (EspHal*)hal;
        hal = nullptr;
    }
}

bool McCompact::setRadioFrequency(float freq) {
    if (radio == nullptr) {
        return false;
    }
    int state = RADIOLIB_ERR_NONE;
    {
        std::lock_guard<std::mutex> lock(mtx_radio);
        state = radio->setFrequency(freq);
    }
    return (state == RADIOLIB_ERR_NONE);
}
bool McCompact::setRadioSpreadingFactor(uint8_t sf) {
    if (radio == nullptr) {
        return false;
    }
    int state = RADIOLIB_ERR_NONE;
    {
        std::lock_guard<std::mutex> lock(mtx_radio);
        switch (radio_type) {
            case RadioType::SX1261:
            case RadioType::SX1262:
            case RadioType::SX1268:
                state = ((SX126x*)radio)->setSpreadingFactor(sf);
                break;
            case RadioType::SX1278:
                state = ((SX1278*)radio)->setSpreadingFactor(sf);
                break;
            case RadioType::SX1276:
                state = ((SX1276*)radio)->setSpreadingFactor(sf);
                break;
            case RadioType::LR1121:
                state = ((LR1121*)radio)->setSpreadingFactor(sf);
                break;
            default:
                state = RADIOLIB_ERR_UNKNOWN;
                break;
        }
        return (state == RADIOLIB_ERR_NONE);
    }
}

bool McCompact::setRadioBandwidth(uint32_t bw) {
    if (radio == nullptr) {
        return false;
    }
    int state = RADIOLIB_ERR_NONE;
    {
        std::lock_guard<std::mutex> lock(mtx_radio);
        switch (radio_type) {
            case RadioType::SX1261:
            case RadioType::SX1262:
            case RadioType::SX1268:
                state = ((SX126x*)radio)->setBandwidth(bw);
                break;
            case RadioType::SX1276:
                state = ((SX1276*)radio)->setBandwidth(bw);
                break;
            case RadioType::SX1278:
                state = ((SX1278*)radio)->setBandwidth(bw);
                break;
            case RadioType::LR1121:
                state = ((LR1121*)radio)->setBandwidth(bw);
                break;
            default:
                state = RADIOLIB_ERR_UNKNOWN;
                break;
        }
        return (state == RADIOLIB_ERR_NONE);
    }
}
bool McCompact::setRadioCodingRate(uint8_t cr) {
    if (radio == nullptr) {
        return false;
    }
    int state = RADIOLIB_ERR_NONE;
    {
        std::lock_guard<std::mutex> lock(mtx_radio);
        switch (radio_type) {
            case RadioType::SX1261:
            case RadioType::SX1262:
            case RadioType::SX1268:
                state = ((SX126x*)radio)->setCodingRate(cr);
                break;
            case RadioType::SX1276:
                state = ((SX1276*)radio)->setCodingRate(cr);
                break;
            case RadioType::SX1278:
                state = ((SX1278*)radio)->setCodingRate(cr);
                break;

            case RadioType::LR1121:
                state = ((LR1121*)radio)->setCodingRate(cr);
                break;
            default:
                state = RADIOLIB_ERR_UNKNOWN;
                break;
        }
        return (state == RADIOLIB_ERR_NONE);
    }
}
bool McCompact::setRadioPower(int8_t power) {
    if (radio == nullptr) {
        return false;
    }
    int state = RADIOLIB_ERR_NONE;
    {
        std::lock_guard<std::mutex> lock(mtx_radio);
        state = radio->setOutputPower(power);
    }
    return (state == RADIOLIB_ERR_NONE);
}

bool McCompact::RadioInit(RadioType radio_type, Radio_PINS& radio_pins, LoraConfig& lora_config) {
    this->radio_type = radio_type;
    if (debugmode) ESP_LOGI(TAG, "RadioInit");
    hal = new EspHal(radio_pins.sck, radio_pins.miso, radio_pins.mosi, radio_pins.cs);
    int state = RADIOLIB_ERR_NONE;
    switch (radio_type) {
        case RadioType::SX1262:
            if (debugmode) ESP_LOGI(TAG, "Using SX1262 radio");
            radio = new SX1262(new Module(hal, radio_pins.cs, radio_pins.irq, radio_pins.rst, radio_pins.gpio));
            state = ((SX1262*)radio)->begin(lora_config.frequency, lora_config.bandwidth, lora_config.spreading_factor, lora_config.coding_rate, lora_config.sync_word, lora_config.output_power, lora_config.preamble_length, lora_config.tcxo_voltage, lora_config.use_regulator_ldo);
            break;
        case RadioType::SX1261:
            if (debugmode) ESP_LOGI(TAG, "Using SX1261 radio");
            radio = new SX1261(new Module(hal, radio_pins.cs, radio_pins.irq, radio_pins.rst, radio_pins.gpio));
            state = ((SX1261*)radio)->begin(lora_config.frequency, lora_config.bandwidth, lora_config.spreading_factor, lora_config.coding_rate, lora_config.sync_word, lora_config.output_power, lora_config.preamble_length, lora_config.tcxo_voltage, lora_config.use_regulator_ldo);
            break;
        case RadioType::SX1268:
            if (debugmode) ESP_LOGI(TAG, "Using SX1268 radio");
            radio = new SX1268(new Module(hal, radio_pins.cs, radio_pins.irq, radio_pins.rst, radio_pins.gpio));
            state = ((SX1268*)radio)->begin(lora_config.frequency, lora_config.bandwidth, lora_config.spreading_factor, lora_config.coding_rate, lora_config.sync_word, lora_config.output_power, lora_config.preamble_length, lora_config.tcxo_voltage, lora_config.use_regulator_ldo);
            break;
        case RadioType::SX1276:
            if (debugmode) ESP_LOGI(TAG, "Using SX1276 radio");
            radio = new SX1276(new Module(hal, radio_pins.cs, radio_pins.irq, radio_pins.rst, radio_pins.gpio));
            state = ((SX1276*)radio)->begin(lora_config.frequency, lora_config.bandwidth, lora_config.spreading_factor, lora_config.coding_rate, lora_config.sync_word, lora_config.output_power, lora_config.preamble_length, 5);
            break;
        case RadioType::SX1278:
            if (debugmode) ESP_LOGI(TAG, "Using SX1278 radio");
            radio = new SX1278(new Module(hal, radio_pins.cs, radio_pins.irq, radio_pins.rst, radio_pins.gpio));
            state = ((SX1278*)radio)->begin(lora_config.frequency, lora_config.bandwidth, lora_config.spreading_factor, lora_config.coding_rate, lora_config.sync_word, lora_config.output_power, lora_config.preamble_length, 5);
            break;

        case RadioType::LR1121:
            if (debugmode) ESP_LOGI(TAG, "Using LR1121 radio");
            radio = new LR1121(new Module(hal, radio_pins.cs, radio_pins.irq, radio_pins.rst, radio_pins.gpio));
            state = ((LR1121*)radio)->begin(lora_config.frequency, lora_config.bandwidth, lora_config.spreading_factor, lora_config.coding_rate, lora_config.sync_word, lora_config.output_power, lora_config.preamble_length, lora_config.tcxo_voltage);
            break;
        default:
            if (debugmode) ESP_LOGW(TAG, "Unsupported radio type, let's try: SX1262");
            radio = new SX1262(new Module(hal, radio_pins.cs, radio_pins.irq, radio_pins.rst, radio_pins.gpio));
            state = ((SX1262*)radio)->begin(lora_config.frequency, lora_config.bandwidth, lora_config.spreading_factor, lora_config.coding_rate, lora_config.sync_word, lora_config.output_power, lora_config.preamble_length, lora_config.tcxo_voltage, lora_config.use_regulator_ldo);
            return false;
    }

    if (state != RADIOLIB_ERR_NONE) {
        if (debugmode) ESP_LOGE(TAG, "failed, code %d\n", state);
        delete hal;
        delete radio;
        hal = nullptr;
        radio = nullptr;
        return false;
    }

    // todo do it less ugly.
    switch (radio_type) {
        case RadioType::SX1261:
            state |= ((SX1261*)radio)->setCurrentLimit(130.0);
            state |= ((SX1261*)radio)->explicitHeader();
            state |= ((SX1261*)radio)->setCRC(RADIOLIB_SX126X_LORA_CRC_ON);
            state |= ((SX1261*)radio)->setDio2AsRfSwitch(false);
            ((SX1261*)radio)->setDio1Action(onPacketReceived);
            state |= ((SX1261*)radio)->setRxBoostedGainMode(true);
            break;
        case RadioType::SX1268:
            state |= ((SX1268*)radio)->setCurrentLimit(130.0);
            state |= ((SX1268*)radio)->explicitHeader();
            state |= ((SX1268*)radio)->setCRC(RADIOLIB_SX126X_LORA_CRC_ON);
            state |= ((SX1268*)radio)->setDio2AsRfSwitch(false);
            ((SX1268*)radio)->setDio1Action(onPacketReceived);
            state |= ((SX1268*)radio)->setRxBoostedGainMode(true);
            break;
        case RadioType::SX1276:
            // todo check
            state |= ((SX1276*)radio)->setCurrentLimit(130.0);
            state |= ((SX1276*)radio)->explicitHeader();
            state |= ((SX1276*)radio)->setCRC(RADIOLIB_SX126X_LORA_CRC_ON);
            // state |= ((SX1276*)radio)->setDio2AsRfSwitch(false);
            ((SX1276*)radio)->setDio0Action(onPacketReceived, 1);
            // state |= ((SX1276*)radio)->setRxBoostedGainMode(true);
            break;
        case RadioType::SX1278:
            // todo check
            state |= ((SX1278*)radio)->setCurrentLimit(130.0);
            state |= ((SX1278*)radio)->explicitHeader();
            state |= ((SX1278*)radio)->setCRC(RADIOLIB_SX126X_LORA_CRC_ON);
            // state |= ((SX1278*)radio)->setDio2AsRfSwitch(false);
            ((SX1278*)radio)->setDio0Action(onPacketReceived, 1);
            // state |= ((SX1278*)radio)->setRxBoostedGainMode(true);
            break;
        case RadioType::LR1121: {
            static const uint32_t rfswitch_dio_pins[] = {
                RADIOLIB_LR11X0_DIO5, RADIOLIB_LR11X0_DIO6,
                RADIOLIB_NC, RADIOLIB_NC, RADIOLIB_NC};
            static const Module::RfSwitchMode_t rfswitch_table[] = {
                // mode                  DIO5  DIO6
                {LR11x0::MODE_STBY, {LOW, LOW}},
                {LR11x0::MODE_RX, {LOW, HIGH}},
                {LR11x0::MODE_TX, {HIGH, LOW}},
                {LR11x0::MODE_TX_HP, {HIGH, LOW}},
                {LR11x0::MODE_TX_HF, {LOW, LOW}},
                {LR11x0::MODE_GNSS, {LOW, LOW}},
                {LR11x0::MODE_WIFI, {LOW, LOW}},
                END_OF_MODE_TABLE,
            };
            ((LR1121*)radio)->setRfSwitchTable(rfswitch_dio_pins, rfswitch_table);
            state |= ((LR1121*)radio)->explicitHeader();
            state |= ((LR1121*)radio)->setCRC(2);
            state |= ((LR1121*)radio)->setRegulatorDCDC();
            state |= ((LR1121*)radio)->setRxBoostedGainMode(true);
            ((LR1121*)radio)->setPacketReceivedAction(onPacketReceived);
            break;
        }
        default:
        case RadioType::SX1262:
            state |= ((SX1262*)radio)->setCurrentLimit(130.0);
            state |= ((SX1262*)radio)->explicitHeader();
            state |= ((SX1262*)radio)->setCRC(RADIOLIB_SX126X_LORA_CRC_ON);
            state |= ((SX1262*)radio)->setDio2AsRfSwitch(false);
            ((SX1262*)radio)->setDio1Action(onPacketReceived);
            state |= ((SX1262*)radio)->setRxBoostedGainMode(true);
            break;
    };
    if (state != 0) {
        ESP_LOGE(TAG, "Radio init failed, code %d\n", state);
        return false;
    }

    RadioListen();    // Start listening for packets
    RadioSendInit();  // Start the send task
    return true;
}

void McCompact::task_send(void* pvParameters) {
    McCompact* mshcomp = static_cast<McCompact*>(pvParameters);
    while (mshcomp->need_run) {
        McPacket_t entry = mshcomp->out_queue.pop();

        if (entry.length == 303) {
            // Stop flag was set, exit the task
            ESP_LOGI(TAG, "Send task stopped");
            continue;
        }
        if (mshcomp->is_send_enabled) {
            {
                packet_during_send = true;  // Indicate that we are currently sending a packet
                std::unique_lock<std::mutex> lock(mshcomp->mtx_radio);
                ESP_LOGE(TAG, "Try send packet. Len: %d", entry.length);
                int err = mshcomp->radio->transmit(entry.payload, entry.length);
                if (err == RADIOLIB_ERR_NONE) {
                    ESP_LOGI(TAG, "Packet sent successfully");
                } else {
                    ESP_LOGE(TAG, "Failed to send packet, code %d", err);
                    vTaskDelay(30 / portTICK_PERIOD_MS);
                    err = mshcomp->radio->transmit(entry.payload, entry.length);
                    if (err == RADIOLIB_ERR_NONE) {
                        ESP_LOGI(TAG, "Packet sent successfully in 2nd try");
                    } else {
                        ESP_LOGE(TAG, "Failed to send packet 2 times in a row, code %d", err);
                    }
                }
                packet_during_send = false;      // Reset the flag after sending
                mshcomp->radio->startReceive();  // Restart receiving after sending
            }
        }
        vTaskDelay(350 / portTICK_PERIOD_MS);  // Wait before next send attempt
    }  // end while
    // never reach here
    vTaskDelete(NULL);
}

void McCompact::task_listen(void* pvParameters) {
    McCompact* mshcomp = static_cast<McCompact*>(pvParameters);
    if (mshcomp->debugmode) ESP_LOGI(pcTaskGetName(NULL), "Start");
    uint8_t rxData[256];  // Maximum Payload size of SX1261/62/68 is 255
    mshcomp->radio->startReceive();
    while (mshcomp->need_run) {
        if (packetFlag) {
            if (!mshcomp->need_run) break;
            packetFlag = false;
            int err = 0;
            int rxLen = 0;
            {
                std::unique_lock<std::mutex> lock(mshcomp->mtx_radio);
                // if (mshcomp->debugmode) ESP_LOGW(TAG, "Packet received, trying to read data");
                rxLen = mshcomp->radio->getPacketLength();
                if (rxLen > 255) rxLen = 255;  // Ensure we do not overflow the buffer
                err = mshcomp->radio->readData(rxData, rxLen);
                mshcomp->rssi = mshcomp->radio->getRSSI();
                mshcomp->snr = mshcomp->radio->getSNR();
                vTaskDelay(1 / portTICK_PERIOD_MS);
                mshcomp->radio->startReceive();
            }
            if (err >= 0) {
                if (mshcomp->onRaw) {
                    mshcomp->onRaw(rxData, rxLen);
                }
                mshcomp->ProcessPacket(rxData, rxLen, mshcomp);
            }

            if (err < 0) {
                if (err == RADIOLIB_ERR_RX_TIMEOUT) {
                    // timeout occurred while waiting for a packet
                    // printf("timeout!\n");
                } else if (err == RADIOLIB_ERR_CRC_MISMATCH) {
                    // packet was received, but is malformed
                    // printf("CRC error!\n");
                } else {
                    // some other error occurred
                    // printf("failed, code %d", err);
                }
            }
        }
        vTaskDelay(20 / portTICK_PERIOD_MS);  // Wait before next receive attempt
    }  // end while
    // never reach here
    vTaskDelete(NULL);
}

bool McCompact::RadioListen() {
    xTaskCreate(&task_listen, "RadioListen", 1024 * 4, this, 5, NULL);
    return true;
}

bool McCompact::RadioSendInit() {
    // Start the send task
    xTaskCreate(&task_send, "RadioSend", 1024 * 4, this, 5, NULL);
    return true;
}

void McCompact::intOnNodeInfo(MCC_Nodeinfo& nodeinfo) {
    nodeinfo_db.addOrUpdate(nodeinfo);
    if (onNodeInfo) {
        onNodeInfo(nodeinfo);
    }
    if (debugmode) ESP_LOGI(TAG, "timestamp=%lu, flags=0x%02x", nodeinfo.timestamp, nodeinfo.flags);
    if (nodeinfo.flags & (uint8_t)MCC_NODEINFO_FLAGS::HAS_LOCATION) {
        if (debugmode) ESP_LOGI(TAG, ", latitude_i=%lu, longitude_i=%lu", nodeinfo.latitude_i, nodeinfo.longitude_i);
    }
    if (nodeinfo.flags & (uint8_t)MCC_NODEINFO_FLAGS::HAS_NAME) {
        if (debugmode) ESP_LOGI(TAG, ", name=%s", nodeinfo.name.c_str());
    }
}

int16_t McCompact::ProcessPacket(uint8_t* data, int len, McCompact* mshcomp) {
    MCC_Header header;
    size_t pos = header.parse(data, len);
    if (pos == 0) {
        if (debugmode) ESP_LOGE(TAG, "Failed to parse MCC header");
        return -1;
    }
    if (debugmode) ESP_LOGI(TAG, "Received packet: route_type=%s, payload_type=%s, addr_format=%d, transport_codes=0x%08" PRIx32 ", path_length=%zu", header.get_route_type_str(), header.get_payload_type_str(), (uint8_t)header.get_payload_version(), header.transport_codes, header.path.path.size());
    if (debugmode) {
        printf("%s: Path: ", TAG);  // Print the tag and header once
        for (const auto& hop : header.path.path) {
            printf(" %lX", hop);  // Print each hop in hex format
        }
        printf("\n");  // Add a single newline at the end
    }
    MCC_PAYLOAD_TYPE plt = header.get_payload_type();

    if (plt == MCC_PAYLOAD_TYPE::PAYLOAD_TYPE_ADVERT) {
        MCC_Nodeinfo nodeinfo;
        if (nodeinfo.parse(data, pos, len) > 0) {
            intOnNodeInfo(nodeinfo);
        }
        return 1;
    }

    if (plt == MCC_PAYLOAD_TYPE::PAYLOAD_TYPE_ACK) {
        uint32_t crc = 0;
        if (len >= pos + 4) {
            crc = *((uint32_t*)&data[pos]);
            if (debugmode) ESP_LOGI(TAG, "ACK for CRC=0x%08" PRIx32, crc);
            return 1;
        }
    }

    if (plt == MCC_PAYLOAD_TYPE::PAYLOAD_TYPE_PATH || plt == MCC_PAYLOAD_TYPE::PAYLOAD_TYPE_REQ || plt == MCC_PAYLOAD_TYPE::PAYLOAD_TYPE_RESPONSE || plt == MCC_PAYLOAD_TYPE::PAYLOAD_TYPE_TXT_MSG) {
        uint8_t dst_hash = *((uint8_t*)&data[pos++]);
        uint8_t src_hash = *((uint8_t*)&data[pos++]);
        uint8_t* macanddata = &data[pos];
        uint8_t datadec[MAX_PACKET_PAYLOAD];
        // todo foreach peer list with that dst hash, and check for secret data if i can decrypt with it.
        uint8_t secret[PUB_KEY_SIZE] = {0};  // 32 bytes
        int lenn = 0;
        for (const auto& peer : mshcomp->nodeinfo_db) {
            memcpy(secret, peer.pubkey, PUB_KEY_SIZE);
            lenn = MACThenDecrypt(secret, datadec, macanddata, len - pos);
            if (lenn > 0) {
                if (debugmode) ESP_LOGI(TAG, "Decrypted payload length: %d", lenn);
                ESP_LOGI(TAG, "Decrypted payload: %s", datadec);
                break;  // Exit the loop if decryption is successful
            }
        }

        if (lenn > 0) {
            if (debugmode) ESP_LOGI(TAG, "Decrypted payload length: %d", lenn);
            ESP_LOGI(TAG, "Decrypted payload: %s", datadec);

        } else {
            if (debugmode) ESP_LOGE(TAG, "Failed to decrypt payload %d", lenn);
            return 0;
        }
        pos = 0;  // inner pos
        if (plt == MCC_PAYLOAD_TYPE::PAYLOAD_TYPE_PATH) {
            if (debugmode) ESP_LOGI(TAG, "PATH packet:NIY");
            return 0;
        }
        if (plt == MCC_PAYLOAD_TYPE::PAYLOAD_TYPE_REQ) {
            // timestamp 4 byte
            uint32_t timestamp = *((uint32_t*)&datadec[pos]);
            pos += 4;
            uint8_t request_type = datadec[pos++];
            if (debugmode) ESP_LOGI(TAG, "REQ packet:NIY");
            return 0;
        }
        if (plt == MCC_PAYLOAD_TYPE::PAYLOAD_TYPE_RESPONSE) {
            // timestamp 4 byte
            uint32_t tag = *((uint32_t*)&datadec[pos]);
            pos += 4;
            if (debugmode) ESP_LOGI(TAG, "RESP packet:NIY");
            return 0;
        }
        if (plt == MCC_PAYLOAD_TYPE::PAYLOAD_TYPE_TXT_MSG) {
            // timestamp 4 byte
            uint32_t timestamp = *((uint32_t*)&datadec[pos]);
            pos += 4;
            uint8_t msg_flags = datadec[pos++];
            uint8_t msg_attempts = msg_flags & 0x03;
            msg_flags = msg_flags >> 2;
            if (msg_flags == 2) {
                // signed_plaintext. remove the first 4 bytes: first four bytes is sender pubkey prefix, followed by plain text message
                pos += 4;
            }
            uint16_t msg_len = len - pos;
            std::string msg = std::string((const char*)&datadec[pos], msg_len);
            if (debugmode) ESP_LOGI(TAG, "TXT_MSG packet: timestamp=%lu, flags=0x%02x, msg_len=%u, msg=%s", timestamp, msg_flags, msg_len, msg.c_str());
            return 1;
            /*
            Received packet of length 38: 09 00 48 AC A0 13 C8 09 C2 36 BF F6 CC 78 B2 35 18 37 75 7D FC 9B 61 F2 0E 41 15 20 4B 52 C0 B2 55 DF 8A 8B E7 65
    I (86779) McCompact: Received packet: route_type=1, payload_type=2, addr_format=0, transport_codes=0x00000000, path_length=0
    I (86799) McCompact: Path:
    I (86799) McCompact: TXT_MSG packet: timestamp=918686152, flags=0xbf, msg_len=27, msg=��x�57u}��a�A KR��Uߊ��e
    Received packet of length 38: 09 00 48 AC 0D 5B 7D C7 81 BF CF 6E 4A 32 00 B8 6A 3E DE E9 F5 B2 61 F2 0E 41 15 20 4B 52 C0 B2 55 DF 8A 8B E7 65
    I (96189) McCompact: Received packet: route_type=1, payload_type=2, addr_format=0, transport_codes=0x00000000, path_length=0
    I (96209) McCompact: Path:
    I (96209) McCompact: TXT_MSG packet: timestamp=3212953469, flags=0xcf, msg_len=27, msg=nJ2
    Received packet of length 38: 09 00 48 AC EC D0 0E 15 8C 29 CC 1F AE C7 C8 58 83 A3 CF 5E 39 0F 61 F2 0E 41 15 20 4B 52 C0 B2 55 DF 8A 8B E7 65
    I (105999) McCompact: Received packet: route_type=1, payload_type=2, addr_format=0, transport_codes=0x00000000, path_length=0
    I (106019) McCompact: Path:
    I (106019) McCompact: TXT_MSG packet: timestamp=697046286, flags=0xcc, msg_len=27, msg=���X���^9a�A KR��Uߊ��e
    */
        }
    }

    if (plt == MCC_PAYLOAD_TYPE::PAYLOAD_TYPE_ANON_REQ) {
        if (debugmode) ESP_LOGI(TAG, "PAYLOAD_TYPE_ANON_REQ NIY");
        return 0;
    }
    if (plt == MCC_PAYLOAD_TYPE::PAYLOAD_TYPE_GRP_TXT) {
        if (debugmode) ESP_LOGI(TAG, "PAYLOAD_TYPE_GRP_TXT");
        uint8_t decoded[MAX_PACKET_PAYLOAD];
        size_t out_decoded_len = 0;
        auto chan = chan_mgr.getChannelByHashAndData(&data[pos], len - pos, decoded, out_decoded_len);
        if (out_decoded_len > 0 && chan) {
            // todo extract other data too
            if (debugmode) ESP_LOGI(TAG, "Decrypted group text length: %zu", out_decoded_len);
            if (debugmode) ESP_LOGI(TAG, "Decrypted group text: %s", decoded + 5);
            if (onGroupMsg) {
                size_t msglen = strnlen((const char*)(decoded + 5), out_decoded_len - 5);
                std::string grpmsg = std::string((const char*)(decoded + 5), msglen);
                uint32_t timestamp = *((uint32_t*)(decoded));
                uint8_t flags = decoded[4];
                onGroupMsg(*chan, grpmsg, timestamp, flags);
            }
        } else {
            if (debugmode) ESP_LOGE(TAG, "Failed to decrypt group text. chanhash: %d", data[pos]);
        }
        return 0;
    }
    if (plt == MCC_PAYLOAD_TYPE::PAYLOAD_TYPE_CONTROL) {
        uint8_t type = data[pos] & 0xF0;
        pos++;
        if (debugmode) ESP_LOGI(TAG, "PAYLOAD_TYPE_CONTROL, type = 0x%02x", type);
        if (type == 0x80 && (pos + 5 <= len)) {
            uint8_t filter = data[pos];                   // bit for each ADV_TYPE_*
            uint32_t tag = *((uint32_t*)&data[pos + 1]);  // random number
            pos += 5;
            uint32_t since = 0;  // optional, epoch
            if (len > pos + 4) {
                since = *((uint32_t*)&data[pos]);
                pos += 4;
            }
            if (debugmode) ESP_LOGI(TAG, "Control packet: Request for node info filter=0x%02x, tag=0x%08" PRIx32 ", since=%lu", filter, tag, since);
            // todo add event
        }
        if (type == 0x90 && (pos + 5 + PUB_KEY_SIZE <= len)) {
            uint8_t snr = data[pos++] / 4;
            uint32_t tag = *((uint32_t*)&data[pos]);
            pos += 4;
            uint8_t pubkey[PUB_KEY_SIZE];
            memcpy(pubkey, &data[pos], PUB_KEY_SIZE);
            pos += PUB_KEY_SIZE;
            if (debugmode) ESP_LOGI(TAG, "Control packet: Response with node info: SNR=%u, tag=0x%08" PRIx32 ", pubkey=%02x%02x", snr, tag, pubkey[0], pubkey[1]);
        }
        return 0;
    }
    return 0;
}

int McCompact::decrypt(const uint8_t* shared_secret, uint8_t* dest, const uint8_t* src, int src_len) {
    if (src_len < 0 || src_len % CIPHER_BLOCK_SIZE != 0 ||
        (src_len > 0 && (shared_secret == nullptr || dest == nullptr || src == nullptr))) {
        ESP_LOGE(TAG, "Invalid AES-ECB input");
        return 0;
    }
    if (src_len == 0) return 0;
    if (!initializePsaCrypto()) return 0;

    psa_key_id_t key_id;
    psa_status_t status = importAesKey(shared_secret, CIPHER_KEY_SIZE, PSA_ALG_ECB_NO_PADDING,
                                       PSA_KEY_USAGE_DECRYPT, &key_id);
    if (status != PSA_SUCCESS) {
        ESP_LOGE(TAG, "AES key import failed: %ld", static_cast<long>(status));
        return 0;
    }

    int processed = 0;
    for (; processed < src_len; processed += CIPHER_BLOCK_SIZE) {
        size_t output_len = 0;
        status = psa_cipher_decrypt(key_id, PSA_ALG_ECB_NO_PADDING, src + processed, CIPHER_BLOCK_SIZE,
                                    dest + processed, CIPHER_BLOCK_SIZE, &output_len);
        if (status != PSA_SUCCESS || output_len != CIPHER_BLOCK_SIZE) {
            if (status == PSA_SUCCESS) status = PSA_ERROR_GENERIC_ERROR;
            ESP_LOGE(TAG, "AES-ECB decryption failed: %ld", static_cast<long>(status));
            break;
        }
    }
    psa_status_t destroy_status = psa_destroy_key(key_id);
    if (destroy_status != PSA_SUCCESS) {
        ESP_LOGE(TAG, "AES key cleanup failed: %ld", static_cast<long>(destroy_status));
        return 0;
    }
    return processed == src_len ? processed : 0;
};
int McCompact::encrypt(const uint8_t* shared_secret, uint8_t* dest, const uint8_t* src, int src_len) {
    if (src_len < 0 || (src_len > 0 && (shared_secret == nullptr || dest == nullptr || src == nullptr))) {
        ESP_LOGE(TAG, "Invalid AES-ECB input");
        return 0;
    }
    if (src_len == 0) return 0;
    if (!initializePsaCrypto()) return 0;

    psa_key_id_t key_id;
    psa_status_t status = importAesKey(shared_secret, CIPHER_KEY_SIZE, PSA_ALG_ECB_NO_PADDING,
                                       PSA_KEY_USAGE_ENCRYPT, &key_id);
    if (status != PSA_SUCCESS) {
        ESP_LOGE(TAG, "AES key import failed: %ld", static_cast<long>(status));
        return 0;
    }

    int input_offset = 0;
    int output_offset = 0;
    while (src_len - input_offset >= CIPHER_BLOCK_SIZE) {
        size_t output_len = 0;
        status = psa_cipher_encrypt(key_id, PSA_ALG_ECB_NO_PADDING, src + input_offset, CIPHER_BLOCK_SIZE,
                                    dest + output_offset, CIPHER_BLOCK_SIZE, &output_len);
        if (status != PSA_SUCCESS || output_len != CIPHER_BLOCK_SIZE) {
            if (status == PSA_SUCCESS) status = PSA_ERROR_GENERIC_ERROR;
            ESP_LOGE(TAG, "AES-ECB encryption failed: %ld", static_cast<long>(status));
            break;
        }
        input_offset += CIPHER_BLOCK_SIZE;
        output_offset += CIPHER_BLOCK_SIZE;
    }
    if (status == PSA_SUCCESS && input_offset < src_len) {
        uint8_t padded_block[CIPHER_BLOCK_SIZE] = {};
        const size_t remainder = static_cast<size_t>(src_len - input_offset);
        memcpy(padded_block, src + input_offset, remainder);
        size_t output_len = 0;
        status = psa_cipher_encrypt(key_id, PSA_ALG_ECB_NO_PADDING, padded_block, sizeof(padded_block),
                                    dest + output_offset, CIPHER_BLOCK_SIZE, &output_len);
        if (status == PSA_SUCCESS && output_len == CIPHER_BLOCK_SIZE) {
            output_offset += CIPHER_BLOCK_SIZE;
        } else {
            if (status == PSA_SUCCESS) status = PSA_ERROR_GENERIC_ERROR;
            ESP_LOGE(TAG, "AES-ECB encryption failed: %ld", static_cast<long>(status));
        }
    }
    psa_status_t destroy_status = psa_destroy_key(key_id);
    if (destroy_status != PSA_SUCCESS) {
        ESP_LOGE(TAG, "AES key cleanup failed: %ld", static_cast<long>(destroy_status));
        return 0;
    }
    return status == PSA_SUCCESS ? output_offset : 0;
};

int McCompact::encryptThenMAC(const uint8_t* shared_secret, uint8_t* dest, const uint8_t* src, int src_len) {
    if (src_len < 0 || shared_secret == nullptr || dest == nullptr || (src_len > 0 && src == nullptr)) {
        ESP_LOGE(TAG, "Invalid AES/HMAC input");
        return 0;
    }
    int enc_len = encrypt(shared_secret, dest + CIPHER_MAC_SIZE, src, src_len);
    if (src_len > 0 && enc_len == 0) return 0;

    uint8_t temp_mac[32];
    if (!computeHmacSha256(shared_secret, dest + CIPHER_MAC_SIZE, static_cast<size_t>(enc_len), temp_mac)) return 0;
    memcpy(dest, temp_mac, CIPHER_MAC_SIZE);
    return enc_len + CIPHER_MAC_SIZE;  // Total length of MAC + ciphertext
};
int McCompact::MACThenDecrypt(const uint8_t* shared_secret, uint8_t* dest, const uint8_t* src, int src_len) {
    if (src_len <= CIPHER_MAC_SIZE || shared_secret == nullptr || dest == nullptr || src == nullptr) {
        ESP_LOGE(TAG, "Invalid HMAC/AES input");
        return 0;  // Invalid source length
    }
    uint8_t calculated_mac[32];  // Buffer to hold the calculated MAC
    const uint8_t* received_mac = src;
    const uint8_t* ciphertext = src + CIPHER_MAC_SIZE;
    const int ciphertext_len = src_len - CIPHER_MAC_SIZE;
    if (!computeHmacSha256(shared_secret, ciphertext, static_cast<size_t>(ciphertext_len), calculated_mac)) return 0;
    if (secure_memcmp(received_mac, calculated_mac, CIPHER_MAC_SIZE) == 0) {
        // 4. If MAC is valid, decrypt the ciphertext.
        return decrypt(shared_secret, dest, ciphertext, ciphertext_len);
    }

    // If MACs do not match, return 0 to indicate authentication failure.
    return 0;
};

int McCompact::secure_memcmp(const void* a, const void* b, size_t size) {
    const unsigned char* a_ptr = (const unsigned char*)a;
    const unsigned char* b_ptr = (const unsigned char*)b;
    unsigned int result = 0;

    // This loop always runs for the full 'size' iterations.
    // The bitwise operations prevent the compiler from optimizing it
    // into a branch that could leak timing information.
    for (size_t i = 0; i < size; i++) {
        result |= a_ptr[i] ^ b_ptr[i];
    }

    return result;
}

void McCompact::sendNodeInfo(MCC_MyNodeInfo& info, bool flood, MCC_Path& path) {
    McPacket_t packet;
    MCC_Header header;
    size_t pos = header.generate_header(&packet, flood ? (uint8_t)MCC_ROUTE_TYPE::ROUTE_TYPE_FLOOD : (uint8_t)MCC_ROUTE_TYPE::ROUTE_TYPE_DIRECT, (uint8_t)MCC_PAYLOAD_TYPE::PAYLOAD_TYPE_ADVERT, path, 0);
    size_t pos2 = info.generate_payload(packet);
    // sign it
    uint8_t message[PUB_KEY_SIZE + 4 + 32 + 32];
    int msg_len = 0;
    memcpy(&message[0], (const void*)&packet.payload[pos], PUB_KEY_SIZE + 4);
    memcpy(&message[PUB_KEY_SIZE + 4], (const void*)&packet.payload[pos + PUB_KEY_SIZE + 4 + 64], pos2 - (PUB_KEY_SIZE + 4 + 64));
    msg_len = pos2 - pos - 64;  // Exclude the signature part
    uint8_t* signature = &packet.payload[pos + PUB_KEY_SIZE + 4];
    info.sign(signature, message, msg_len);
    // try to send it
    if (out_queue.push(packet)) {
        if (debugmode) ESP_LOGI(TAG, "Nodeinfo  sent");
    } else {
        if (debugmode) ESP_LOGE(TAG, "Failed to send nodeinfo, queue full");
    }
}
void McCompact::sendGroupMsg(const MCC_ChannelEntry& channel, const std::string& sender, const std::string& msg, MCC_Path& path) {
    McPacket_t packet;
    MCC_Header header;
    size_t pos = header.generate_header(&packet, (uint8_t)MCC_ROUTE_TYPE::ROUTE_TYPE_FLOOD, (uint8_t)MCC_PAYLOAD_TYPE::PAYLOAD_TYPE_GRP_TXT, path, 0);
    uint32_t timestamp = (uint32_t)time(NULL);
    packet.payload[pos++] = channel.hash[0];
    std::string text_part = sender + ": " + msg;

    std::vector<uint8_t> message;

    // Opcionális, de jó gyakorlat teljesítményszempontból: előre lefoglaljuk a memóriát
    message.reserve(4 + 1 + text_part.length());

    // 4 bájt timestamp betöltése (Little Endian sorrendben)
    message.push_back(timestamp & 0xFF);
    message.push_back((timestamp >> 8) & 0xFF);
    message.push_back((timestamp >> 16) & 0xFF);
    message.push_back((timestamp >> 24) & 0xFF);

    // Flag betöltése (bit0-1: attempts, bit2: signed/unsigned)
    message.push_back(0x00);
    // A szöveges üzenet bájtonkénti hozzáfűzése a vektor végéhez
    message.insert(message.end(), text_part.begin(), text_part.end());
    // Titkosítás hívása
    int enlen = McCompact::encryptThenMAC(
        channel.secret,
        &packet.payload[pos],
        message.data(),
        message.size());
    pos += enlen;
    packet.length = pos;
    if (out_queue.push(packet)) {
        if (debugmode) ESP_LOGI(TAG, "Group message sent");
    } else {
        if (debugmode) ESP_LOGE(TAG, "Failed to send group message, queue full");
    }
}

void McCompact::sendNeighborDiscoveryRequest(uint8_t filter) {
    McPacket_t packet;
    MCC_Header header;
    MCC_Path path;  // Empty path for direct routing
    // size_t generate_header(McPacket_t* packet, uint8_t route_type,  uint8_t payload_type, std::vector<uint32_t> path, uint8_t path_bytenum = 1, uint32_t transport_code = 0) {
    header.generate_header(&packet, (uint8_t)MCC_ROUTE_TYPE::ROUTE_TYPE_DIRECT, (uint8_t)MCC_PAYLOAD_TYPE::PAYLOAD_TYPE_CONTROL, path, 0);
    uint32_t tag = (uint32_t)random();
    packet.payload[packet.length++] = 0x80;  // control packet type: request for node data
    packet.payload[packet.length++] = filter;
    packet.payload[packet.length++] = (tag >> 24) & 0xFF;
    packet.payload[packet.length++] = (tag >> 16) & 0xFF;
    packet.payload[packet.length++] = (tag >> 8) & 0xFF;
    packet.payload[packet.length++] = tag & 0xFF;
    packet.payload[packet.length++] = 0;  // optional since uint32_t
    packet.payload[packet.length++] = 0;
    packet.payload[packet.length++] = 0;
    packet.payload[packet.length++] = 0;
    // try to send it
    if (out_queue.push(packet)) {
        if (debugmode) ESP_LOGI(TAG, "Neighbor discovery request sent, tag=0x%08" PRIx32, tag);
    } else {
        if (debugmode) ESP_LOGE(TAG, "Failed to send neighbor discovery request, queue full");
    }
}