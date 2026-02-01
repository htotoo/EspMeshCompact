# Meshtastic ESP32S3 Compact Component

This is a **proof-of-concept (POC)** compact Meshtastic + MeshCore component for the ESP32S3.

## Overview

- Minimal implementation for ESP32S3 hardware.
- Intended for experimentation and development.
- Not production ready.

## Features

- Basic Meshtastic protocol support.
- Basic MeshCore protocol support. (WIP)

## Requirements

- ESP32S3 development board
- ESP-IDF >5
- RadioLib

## Disclaimer

This project is for **POC and educational purposes only**. Use at your own risk.

## Examples

This lib is used in some projects, so those are for usage examples.
- https://github.com/htotoo/meshbaba - a simple Mesh notificator, and Ping responder.
- https://github.com/htotoo/DarkMesh - a complex app that can abuse the MT's weak points for demonstration.
- https://github.com/htotoo/TPagerMesh - a work in progress / abandoned T-Pager FW using this lib for communication.


## Using this project needs these cmake options:
```
add_compile_definitions(RADIOLIB_EXCLUDE_RF69)
add_compile_definitions(RADIOLIB_EXCLUDE_SX1231)
add_compile_definitions(RADIOLIB_EXCLUDE_AFSK)
add_compile_definitions(RADIOLIB_EXCLUDE_APRS)
add_compile_definitions(RADIOLIB_EXCLUDE_AX25)
add_compile_definitions(RADIOLIB_EXCLUDE_BELL)
add_compile_definitions(RADIOLIB_EXCLUDE_FSK4)    
add_compile_definitions(RADIOLIB_EXCLUDE_HELLSCHREIBER)
add_compile_definitions(RADIOLIB_EXCLUDE_MORSE)
add_compile_definitions(RADIOLIB_EXCLUDE_PAGER)
add_compile_definitions(RADIOLIB_EXCLUDE_RTTY)
add_compile_definitions(RADIOLIB_EXCLUDE_SSTV)
```
## WIP
This project is under active development and subject to change. Many features are incomplete or not yet implemented. Contributions and pull requests are welcome!
