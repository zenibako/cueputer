# Cueputer

**A theater tech multitool firmware for the M5Stack Cardputer ADV.**

Cueputer turns the Cardputer ADV into a pocket-sized production toolkit — an OSC tester, wireless MIDI bridge, audio test generator, and show companion, all in one card-sized device.

Part of the [Zenibako](https://github.com/zenibako) theater tech ecosystem alongside [Cuejitsu](https://github.com/zenibako/cuejitsu) and [qlab-golang](https://github.com/zenibako/qlab-golang).

## Features

### 🎛️ OSC Tester
- Send and receive OSC messages over WiFi (UDP)
- Type OSC addresses and arguments on the 56-key keyboard
- Live response display on the 1.14" LCD
- Network scanner to discover OSC endpoints
- Saved message templates on microSD
- QLab-aware shortcuts (Go, Stop, Panic, cue status)
- ETC EOS support (channel check, intensity levels)

### 🎹 Wireless MIDI Bridge
- BLE MIDI controller — pair with QLab, DAWs, or any BLE MIDI host
- Keyboard-as-MIDI: trigger notes/CCs for testing
- BLE MIDI → WiFi OSC bridge mode (translate MIDI to OSC in real time)

### 🎧 Audio Tools
- Pink noise and test tone generator (via 3.5mm out)
- Sine, white noise, pink noise, sweep
- SPL meter using the built-in MEMS microphone
- Speaker check utility

### 💡 Bonus Modules
- sACN/Art-Net monitor — sniff DMX universes, display channel values
- SMPTE/MTC timecode display
- Show caller's companion — browse Cuejitsu cue list, trigger Go via OSC

## Hardware

**M5Stack Cardputer ADV** (ESP32-S3)
- ESP32-S3FN8 dual-core @ 240MHz
- 1.14" LCD (240×135px)
- 56-key keyboard
- ES8311 audio codec + 1W speaker + MEMS mic + 3.5mm out
- WiFi + Bluetooth 5.0 (BLE)
- BMI270 6-axis IMU
- IR emitter, microSD slot
- 1750mAh battery
- Grove + 14-pin expansion

## Tech Stack

- **Framework:** Arduino / PlatformIO
- **Language:** C++
- **Architecture:** Modular menu system — each tool is an independent module
- **Libraries:**
  - [CNMAT/OSC](https://github.com/CNMAT/OSC) — OSC send/receive
  - [ESP32-BLE-MIDI](https://github.com/max22-/ESP32-BLE-MIDI) — BLE MIDI
  - [M5Unified](https://github.com/m5stack/M5Unified) — M5Stack hardware abstraction
  - [ESP32-audioI2S](https://github.com/schreibfaul1/ESP32-audioI2S) or raw I2S — audio generation

## Project Structure

```
cueputer/
├── src/
│   ├── main.cpp              # Entry point, module launcher
│   ├── ui/
│   │   ├── menu.h/cpp        # Main menu system
│   │   ├── display.h/cpp     # LCD helpers
│   │   └── keyboard.h/cpp    # Key input handling
│   ├── modules/
│   │   ├── osc_tester/       # OSC send/receive/discover
│   │   ├── midi_bridge/      # BLE MIDI controller + bridge
│   │   ├── audio_tools/      # Tone gen, SPL meter
│   │   ├── dmx_monitor/      # sACN/Art-Net sniffer
│   │   └── show_companion/   # Cue list viewer + OSC trigger
│   ├── net/
│   │   ├── wifi_manager.h    # WiFi connection management
│   │   └── osc_client.h      # OSC UDP client/server
│   └── config/
│       └── settings.h        # Persistent settings (microSD/NVS)
├── platformio.ini
├── README.md
└── data/                     # microSD default files (templates, etc.)
```

## Getting Started

```bash
# Clone
git clone https://github.com/zenibako/cueputer.git
cd cueputer

# Build and flash (PlatformIO)
pio run -t upload

# Monitor serial output
pio device monitor
```

## License

MIT
