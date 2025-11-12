#pragma once
#include <stdint.h>

enum class RadioType {
    SX1262,
    SX1261,
    SX1268,
    SX1276
};

struct Radio_PINS {
    uint8_t sck;
    uint8_t miso;
    uint8_t mosi;
    uint8_t cs;
    uint8_t irq;
    uint8_t rst;
    uint8_t gpio;
};

class LoraConfig {
   public:
    LoraConfig() {};
    LoraConfig(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t sw, uint16_t pl, int8_t op, float tv, bool ldo)
        : frequency(freq),
          bandwidth(bw),
          spreading_factor(sf),
          coding_rate(cr),
          sync_word(sw),
          preamble_length(pl),
          output_power(op),
          tcxo_voltage(tv),
          use_regulator_ldo(ldo) {};
    LoraConfig(float freq, int8_t output_power = 22, float tcxo_voltage = 1.8, bool use_regulator_ldo = false)
        : frequency(freq),
          output_power(output_power),
          tcxo_voltage(tcxo_voltage),
          use_regulator_ldo(use_regulator_ldo) {};

    float frequency = 869.525;       // Frequency in MHz
    float bandwidth = 250.0;         // Bandwidth in kHz
    uint8_t spreading_factor = 8;    // Spreading factor (7-12)
    uint8_t coding_rate = 11;        // Coding rate denominator (5-8)
    uint8_t sync_word = 0x2b;        // Sync word
    uint16_t preamble_length = 16;   // Preamble length in symbols
    int8_t output_power = 22;        // Output power in dBm
    float tcxo_voltage = 1.8;        // TCXO voltage in volts
    bool use_regulator_ldo = false;  // Use LDO regulator (true) or DC-DC regulator (false)
};
