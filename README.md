# Weather Web Server

An ESP32-based weather station that reads temperature (DS18B20), humidity
(DHT11), and barometric pressure (BMP280) and serves a live dashboard over
your home WiFi.

## Hardware

- ESP32 dev board (any variant with I2C + a couple of free GPIOs)
- BMP280 breakout board (barometric pressure, I2C)
- DS18B20 (temperature, OneWire) — waterproof probe or TO-92 package
- DHT11 module (humidity)
- 4.7kΩ resistor (pull-up for the DS18B20 data line, unless your breakout
  already has one built in)

### Wiring

| Sensor pin       | ESP32 pin |
|-------------------|-----------|
| BMP280 VIN         | 3V3       |
| BMP280 GND         | GND       |
| BMP280 SCL         | GPIO 22   |
| BMP280 SDA         | GPIO 21   |
| DS18B20 VDD        | 3V3       |
| DS18B20 GND        | GND       |
| DS18B20 DATA       | GPIO 4    |
| DHT11 VCC          | 3V3       |
| DHT11 GND          | GND       |
| DHT11 DATA         | GPIO 27   |

The DS18B20 needs a 4.7kΩ pull-up resistor between its DATA and VDD lines
(many breakout boards already include one).

Pins are configurable in `include/config.h`:
- `SDA_PIN` / `SCL_PIN` — BMP280 I2C
- `ONE_WIRE_PIN` — DS18B20
- `DHT_PIN` — DHT11

The BMP280 is typically at I2C address `0x76` or `0x77` — the firmware tries
both, but set `BMP280_ADDRESS` in `config.h` if you know yours.

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

- Reads all three sensors every 5 seconds (`SENSOR_READ_INTERVAL_MS` in
  `config.h`). Temperature comes from the DS18B20, humidity from the DHT11,
  and pressure/altitude from the BMP280 — each tracked independently so one
  sensor failing doesn't blank out the others.
- Keeps a rolling 24-hour history in memory (sampled every 5 minutes) for the
  temperature trend chart.
- Serves a dashboard at `/` with live temperature, humidity, pressure, and
  altitude, auto-refreshing every 5 seconds. Any sensor that isn't detected
  shows `--` and is called out in the status line.
- Exposes JSON APIs:
  - `GET /api/current` — latest reading, with `temp_valid` / `humidity_valid`
    / `pressure_valid` flags per sensor
  - `GET /api/history` — recent history points for charting

## Project layout

```
include/
  config.h            Sensor pins, addresses, and timing constants
  secrets.h.example    Template for WiFi credentials (copy to secrets.h)
  weather_sensor.h      Sensor wrapper interface (BMP280 + DS18B20 + DHT11)
src/
  main.cpp             WiFi, web server, REST API, history buffer
  weather_sensor.cpp    Per-sensor read logic
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
- **Swapping a sensor**: each measurement is read independently in
  `weather_sensor.cpp`, so you can swap out any one module (e.g. DHT11 for a
  DHT22) without touching the web server or dashboard.
