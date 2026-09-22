# Weather Web Server

An ESP32-based weather station that reads temperature, humidity, and barometric
pressure from a BME280 sensor and serves a live dashboard over your home WiFi.

## Hardware

- ESP32 dev board (any variant with I2C exposed)
- BME280 breakout board (temperature + humidity + barometric pressure)
- 4 jumper wires

### Wiring (I2C)

| BME280 pin | ESP32 pin        |
|------------|------------------|
| VIN        | 3V3              |
| GND        | GND              |
| SCL        | GPIO 22          |
| SDA        | GPIO 21          |

Pins are configurable in `include/config.h` (`SDA_PIN` / `SCL_PIN`). The BME280
is typically at I2C address `0x76` or `0x77` — the firmware tries both, but
set `BME280_ADDRESS` in `config.h` if you know yours.

## Software setup

1. Install [PlatformIO](https://platformio.org/) (VS Code extension or CLI).
2. Copy the secrets template and fill in your WiFi credentials:

   ```sh
   cp include/secrets.h.example include/secrets.h
   ```

3. Build and flash the firmware:

   ```sh
   pio run --target upload
   ```

4. Upload the web dashboard (from `data/`) to the ESP32's filesystem:

   ```sh
   pio run --target uploadfs
   ```

5. Open the serial monitor to find the assigned IP address, or just visit
   `http://weather.local` (mDNS hostname, configurable via `MDNS_HOSTNAME` in
   `secrets.h`).

   ```sh
   pio device monitor
   ```

## What it does

- Reads the BME280 every 5 seconds (`SENSOR_READ_INTERVAL_MS` in `config.h`).
- Keeps a rolling 24-hour history in memory (sampled every 5 minutes) for the
  temperature trend chart.
- Serves a dashboard at `/` with live temperature, humidity, pressure, and
  altitude, auto-refreshing every 5 seconds.
- Exposes JSON APIs:
  - `GET /api/current` — latest sensor reading
  - `GET /api/history` — recent history points for charting

## Project layout

```
include/
  config.h            Sensor pins, addresses, and timing constants
  secrets.h.example    Template for WiFi credentials (copy to secrets.h)
  weather_sensor.h      Sensor wrapper interface
src/
  main.cpp             WiFi, web server, REST API, history buffer
  weather_sensor.cpp    BME280 read logic
data/
  index.html           Dashboard served from SPIFFS
platformio.ini          Board, framework, and library dependencies
```

## Customizing

- **Altitude accuracy**: set `SEA_LEVEL_PRESSURE_HPA` in `config.h` to your
  local sea-level-adjusted pressure (check a nearby weather station) for a
  more accurate altitude reading.
- **History resolution**: adjust `HISTORY_SIZE` and `HISTORY_SAMPLE_INTERVAL_MS`
  in `config.h` — defaults keep 24 hours at 5-minute resolution.
- **Different sensor**: if you're using a DHT22 + BMP280 instead of a BME280,
  swap the implementation in `weather_sensor.cpp`/`.h` — the rest of the app
  (web server, JSON API, dashboard) is sensor-agnostic.
