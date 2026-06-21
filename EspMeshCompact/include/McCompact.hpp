#pragma once

#include <stdio.h>
#include <inttypes.h>
#include <ctype.h>
#include <string.h>
#include "RadioStructs.hpp"
#include "RadioLib.h"
#include "EspHal.h"
#include "esp_random.h"
#include "mbedtls/aes.h"
#include "mbedtls/md.h"
#include <string>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <deque>
#include "McCompactStructs.hpp"
#include "McCompactNodeInfoDB.hpp"
#include "McCompatChanMgr.hpp"
#include "McCompactOutQueue.hpp"
#include "mbedtls/constant_time.h"

#define MAX_PACKET_PAYLOAD 184
#define PUB_KEY_SIZE 32
#define PRV_KEY_SIZE 64
#define SEED_SIZE 32
#define SIGNATURE_SIZE 64
#define CIPHER_KEY_SIZE 16
#define CIPHER_BLOCK_SIZE 16
#define CIPHER_MAC_SIZE 2

class McCompact {
   public:
    McCompact();
    ~McCompact();
    bool RadioInit(RadioType radio_type, Radio_PINS& radio_pins, LoraConfig& lora_config);  // Initializes the radio with the given configuration and pins

    using OnRaw = void (*)(const uint8_t* data, size_t len);
    using OnNodeInfo = void (*)(const MCC_Nodeinfo& info);
    using OnGroupMsg = void (*)(const MCC_ChannelEntry& channel, const std::string& msg);  // todo extract other data

    void setOnRaw(OnRaw cb) { onRaw = cb; }
    void setOnNodeInfo(OnNodeInfo cb) { onNodeInfo = cb; }
    void setOnGroupMsg(OnGroupMsg cb) { onGroupMsg = cb; }

    void getLastSignalData(float& rssi_out, float& snr_out) {
        rssi_out = rssi;
        snr_out = snr;
    }

    // Radio settings on the fly
    bool setRadioFrequency(float freq);
    bool setRadioSpreadingFactor(uint8_t sf);
    bool setRadioBandwidth(uint32_t bw);
    bool setRadioCodingRate(uint8_t cr);
    bool setRadioPower(int8_t power);

    NodeInfoCoreDB nodeinfo_db{};
    McCompatChanMgr chan_mgr{};

    static int decrypt(const uint8_t* shared_secret, uint8_t* dest, const uint8_t* src, int src_len);
    static int encrypt(const uint8_t* shared_secret, uint8_t* dest, const uint8_t* src, int src_len);
    static int encryptThenMAC(const uint8_t* shared_secret, uint8_t* dest, const uint8_t* src, int src_len);
    static int MACThenDecrypt(const uint8_t* shared_secret, uint8_t* dest, const uint8_t* src, int src_len);
    static int secure_memcmp(const void* a, const void* b, size_t size);

    // To enable or disable this module's logging to serial
    void setDebugMode(bool enabled) {
        debugmode = enabled;
    }
    /**
     * @brief Set the Send Enabled
     *
     * @param enabled If set to false, no packets will be sent, but we still receive them.
     */
    void setSendEnabled(bool enabled) {
        is_send_enabled = enabled;
    }

    // nodeinfo settings

    void setMyNames(const std::string& name) {
        my_nodeinfo.name = name;
    }
    void setMyLocation(float latitude, float longitude) {
        my_nodeinfo.latitude_i = static_cast<uint32_t>(latitude * 1e6);
        my_nodeinfo.longitude_i = static_cast<uint32_t>(longitude * 1e6);
        my_nodeinfo.has_location = true;
    }

    void setMyPrivateKey(const uint8_t* priv_key) {
        my_nodeinfo.setPrivateKey(priv_key);
    }
    void generateMyKeyPair() {
        my_nodeinfo.generateKeyPair();
    }

    void setMyTypeFlags(MCC_NODEINFO_FLAGS flag) {  // MCC_NODEINFO_FLAGS::IS_CHAT_NODE,  ... only 1
        my_nodeinfo.flags = (uint8_t)flag;
    }

    MCC_MyNodeInfo* getMyNodeInfo() {
        return &my_nodeinfo;
    }

    // senders
    void sendNodeInfo(const MCC_MyNodeInfo& info);
    void sendMyNodeInfo() {
        sendNodeInfo(my_nodeinfo);
    }
    void sendGroupMsg(const MCC_ChannelEntry& channel, const std::string& msg);
    void sendNeighborDiscoveryRequest(uint8_t filter = 15, std::vector<uint32_t> path = {});

   private:
    RadioType radio_type;
    bool RadioListen();    // inits the listening thread for the radio
    bool RadioSendInit();  // inits the sending thread for the radio. consumes the out_queue
                           // handlers

    // internal events
    void intOnNodeInfo(MCC_Nodeinfo& info);  // internal handler for nodeinfo packets

    // decoding
    int16_t ProcessPacket(uint8_t* data, int len, McCompact* mshcomp);  // Process the packet, decode it, and call the appropriate handler

    static void task_listen(void* pvParameters);  // Task for listening to the radio and processing incoming packets
    static void task_send(void* pvParameters);    // Task for sending packets from the out_queue

    float rssi, snr;  // store last signal data

    EspHal* hal;           // = new EspHal(9, 11, 10);
    PhysicalLayer* radio;  // SX1262 radio = new Module(hal, 8, 14, 12, 13);

    mutable std::mutex mtx_radio;
    bool need_run = true;  // thread exit flag

    bool debugmode = false;  // if true, enables debug logging

    bool is_send_enabled = true;  // if false, disables sending of packets

    MCC_MyNodeInfo my_nodeinfo;
    OnRaw onRaw = nullptr;
    OnNodeInfo onNodeInfo = nullptr;
    OnGroupMsg onGroupMsg = nullptr;

    McCompactOutQueue out_queue;  // Outgoing queue for packets to be sent
};
