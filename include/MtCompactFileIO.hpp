#pragma once

#include "MtCompactNodeInfoDB.hpp"

class MtCompactFileIO {
   public:
    static const uint8_t FILEIO_VERSION = 1;
    // Save the nodedb
    static bool saveNodeDb(NodeInfoDB& db);

    // Load the nodedb
    static bool loadNodeDb(NodeInfoDB& db);

    // Save the private key
    static bool savePrivateKey(MCT_MyNodeInfo& my_nodeinfo);
    // Load the private key
    static bool loadPrivateKey(MCT_MyNodeInfo& my_nodeinfo);
};