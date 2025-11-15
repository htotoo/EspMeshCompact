
#include "MtCompactFileIO.hpp"
#include <nvs_flash.h>
#include <nvs.h>
#include <vector>
#include "esp_log.h"

bool MtCompactFileIO::saveNodeDb(NodeInfoDB& db) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open("meshtastic", NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return false;
    }

    // Save FILEIO_VERSION to NVS
    err = nvs_set_u32(handle, "fileio_ver", FILEIO_VERSION);
    if (err != ESP_OK) {
        nvs_close(handle);
        return false;
    }

    // Serialize db to a buffer
    std::vector<uint8_t> buffer;
    if (!db.serialize(buffer)) {
        nvs_close(handle);
        return false;
    }

    // Write buffer to NVS
    err = nvs_set_blob(handle, "nodedb", buffer.data(), buffer.size());
    if (err != ESP_OK) {
        nvs_close(handle);
        return false;
    }

    err = nvs_commit(handle);
    nvs_close(handle);

    return err == ESP_OK;
    return false;
}

bool MtCompactFileIO::loadNodeDb(NodeInfoDB& db) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open("meshtastic", NVS_READONLY, &handle);
    if (err != ESP_OK) {
        return false;
    }

    uint32_t fileio_ver = 0;
    err = nvs_get_u32(handle, "fileio_ver", &fileio_ver);
    if (err != ESP_OK || fileio_ver != FILEIO_VERSION) {
        nvs_close(handle);
        ESP_LOGI("MtCompactFileIO", "NVS fileio_ver mismatch or not found.");
        return false;
    }

    size_t required_size = 0;
    err = nvs_get_blob(handle, "nodedb", nullptr, &required_size);
    if (err != ESP_OK || required_size == 0) {
        nvs_close(handle);
        return false;
    }

    std::vector<uint8_t> buffer(required_size);
    err = nvs_get_blob(handle, "nodedb", buffer.data(), &required_size);
    nvs_close(handle);
    if (err != ESP_OK) {
        return false;
    }

    return db.deserialize(buffer);
    return false;
}

bool MtCompactFileIO::savePrivateKey(MCT_MyNodeInfo& my_nodeinfo) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open("mtpriv", NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE("MtCompactFileIO", "Failed to open NVS namespace 'mtpriv' for writing private key. Err: %d", err);
        return false;
    }

    err = nvs_set_blob(handle, "priv_key", my_nodeinfo.private_key, 32);
    if (err != ESP_OK) {
        ESP_LOGE("MtCompactFileIO", "Failed to write private key to NVS. Err: %d", err);
        nvs_close(handle);
        return false;
    }

    err = nvs_commit(handle);
    nvs_close(handle);

    return err == ESP_OK;
}

bool MtCompactFileIO::loadPrivateKey(MCT_MyNodeInfo& my_nodeinfo) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open("mtpriv", NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGE("MtCompactFileIO", "Failed to open NVS namespace 'mtpriv' for reading private key. Err: %d", err);
        return false;
    }

    size_t required_size = 32;
    err = nvs_get_blob(handle, "priv_key", my_nodeinfo.private_key, &required_size);
    nvs_close(handle);
    if (err != ESP_OK) {
        ESP_LOGE("MtCompactFileIO", "Failed to read private key from NVS. Err: %d", err);
        return false;
    }

    return true;
}