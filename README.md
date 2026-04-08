# IoT Environmental Monitoring Station

FreeRTOS-based firmware for Arduino Uno R3 that reads three environmental sensors concurrently and outputs data via Serial Monitor. 

---

## Hardware

| Component | Interface | Pin |
|-----------|-----------|-----|
| DHT22 / AM2302 (temp + humidity) | One-wire | Digital 2 |
| BH1750FVI (light intensity) | I2C | SDA=A4, SCL=A5 |
| HC-SR501 PIR (motion) | Digital GPIO | Digital 3 |
| Serial output | USB | TX=1, RX=0 |

See [docs/wiring.md](docs/wiring.md) for the full wiring diagram and hardware notes.

---

## Architecture

### FreeRTOS Tasks

```
Priority 3 (highest): task_serial      — reads queue, formats output to Serial
Priority 2:           task_dht22       — reads temperature + humidity every 2s
Priority 2:           task_environment — reads BH1750 every 1s, PIR every 250ms
Priority 1 (lowest):  idle task        — built-in FreeRTOS idle
```

- `task_serial` is the **only** task that calls `Serial.print()` — no mutex on Serial needed.
- BH1750 and PIR share one task to save ~128 bytes of stack vs. a dedicated PIR task.
- The I2C bus is protected by `i2c_mutex` even though only BH1750 uses it, to demonstrate correct resource protection and allow future I2C sensors.

### Inter-task Communication

```
task_dht22       ──→ xQueueSend(sensor_data_queue) ──→ task_serial
task_environment ──→ xQueueSend(sensor_data_queue) ──→ task_serial

I2C bus: xSemaphoreTake(i2c_mutex) / xSemaphoreGive(i2c_mutex)
```

### Memory Budget (2048 bytes SRAM)

```
FreeRTOS kernel overhead:    ~400 bytes
task_dht22 stack:            128 words = 256 bytes
task_environment stack:      128 words = 256 bytes
task_serial stack:           150 words = 300 bytes
Idle task stack:              92 words = 184 bytes
sensor_data_queue:           8 items × 8 bytes = 64 bytes
I2C mutex:                   ~80 bytes
Global variables + buffers:  ~150 bytes
─────────────────────────────────────────────────────
Estimated total:             ~1690 bytes  (358 bytes headroom)
```

---

## Serial Output

```
[00042] TEMP: 23.50 C
[00042] HUM:  61.20 %
[00043] LIGHT: 342 lux
[00043] MOTION: detected
```

Format: `[timestamp_seconds] TYPE: value unit`

---

## Project Structure

```
iot-monitoring-station/
├── platformio.ini
├── include/
│   ├── types.h          # sensor_reading_t, SensorType
│   ├── shared.h         # extern queue + mutex handles
│   ├── dht22.h
│   ├── bh1750.h
│   └── pir.h
├── src/
│   ├── main.cpp         # FreeRTOS setup, task creation, scheduler start
│   ├── config/
│   │   └── pins.h       # Pin definitions — no magic numbers
│   ├── drivers/
│   │   ├── dht22.cpp
│   │   ├── bh1750.cpp
│   │   └── pir.cpp
│   └── tasks/
│       ├── task_dht22.cpp
│       ├── task_environment.cpp
│       └── task_serial.cpp
└── docs/
    └── wiring.md
```

---

## Build & Run

Requires [PlatformIO](https://platformio.org/) (VS Code extension or CLI). **Do not use Arduino IDE.**

```bash
# Build
pio run

# Upload to board
pio run --target upload

# Open Serial Monitor (115200 baud)
pio device monitor --baud 115200

# Clean build artifacts
pio run --target clean
```

---

## Tech Stack

- **Language:** C++17
- **RTOS:** FreeRTOS (`feilipu/FreeRTOS@^11.1.0`)
- **Build system:** PlatformIO
- **Board:** Arduino Uno R3 (ATmega328P, 2KB SRAM, 32KB Flash)
- **Libraries:**
  - `adafruit/DHT sensor library@^1.4.6`
  - `adafruit/Adafruit Unified Sensor@^1.1.14`
  - `claws/BH1750@^1.3.0`

---

## Phase 2 (planned)

- Alert system: LED on pin 13 blinks on threshold violations
- Unit tests with Unity framework (PlatformIO native environment)
- Binary serial protocol with CRC8 for Raspberry Pi communication
- Raspberry Pi data receiver + backend API (FastAPI, PostgreSQL, dashboard)
