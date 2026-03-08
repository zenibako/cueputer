# data/

Files in this directory are uploaded to the ESP32's SPIFFS/LittleFS filesystem via:

```bash
pio run -t uploadfs
```

## Contents (planned)

- `osc_templates.json` — saved OSC message templates
- `midi_mappings.json` — saved keyboard→MIDI note mappings
- `settings.json` — fallback settings (primary storage is NVS)

> Note: WiFi credentials are stored in NVS (not here) for security.
