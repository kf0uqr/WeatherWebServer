# Weather Web Server

An ESP32-based weather station that reads temperature (DS18B20), humidity
(DHT11), barometric pressure (BMP280), and rainfall (FC-37/YL-83 rain drop
sensor), and serves a live dashboard over your home WiFi.

## Hardware

- ESP32 dev board (any variant with I2C + a few free GPIOs, two of them
  ADC-capable)
- BMP280 breakout board (barometric pressure, I2C)
- DS18B20 (temperature, OneWire) — waterproof probe or TO-92 package
- DHT11 module (humidity)
- FC-37 / YL-83 style rain drop sensor (analog + digital output board)
- 4.7kΩ resistor (pull-up for the DS18B20 data line, unless your breakout
  already has one built in)

### Wiring

| Sensor pin        | ESP32 pin |
|--------------------|-----------|
| BMP280 VCC         | 3V3       |
| BMP280 GND         | GND       |
| BMP280 SCK         | GPIO 22   |
| BMP280 SDI         | GPIO 21   |
| BMP280 SDO         | GND       |
| BMP280 CS          | 3V3       |
| DS18B20 VDD        | 3V3       |
| DS18B20 GND        | GND       |
| DS18B20 DATA       | GPIO 4    |
| DHT11 VCC          | 3V3       |
| DHT11 GND          | GND       |
| DHT11 DATA         | GPIO 27   |
| Rain sensor VCC    | 3V3       |
| Rain sensor GND    | GND       |
| Rain sensor AO     | GPIO 34   |
| Rain sensor DO     | GPIO 35   |

The DS18B20 needs a 4.7kΩ pull-up resistor between its DATA and VDD lines
(many breakout boards already include one).

Boards with a `VCC`/`3.3V`/`GND`/`SCK`/`SDO`/`SDI`/`CS` pinout support both
I2C and SPI — **CS** picks the mode. Tying `CS` high (to 3V3) puts it in I2C
mode, which is what this firmware talks to (`Wire`/`Adafruit_BMP280`), so no
code changes are needed. In that mode `SCK` and `SDI` become the I2C clock
and data lines (`SCL`/`SDA`), and `SDO` sets the I2C address: tie it to
`GND` for `0x76` (the default in `config.h`) or to `3V3` for `0x77`. Leave
your board's separate `3.3V` pin (a regulator output some boards break out)
unconnected — power the board from `VCC` only.

The rain sensor board has two outputs: **AO** (analog voltage that varies
with how wet the board is) and **DO** (digital, flips when wetness crosses
a threshold set by the onboard potentiometer). Both are wired in so the
dashboard gets a rain intensity percentage and a simple rain/no-rain flag.
GPIO 34/35 are ADC1-capable input-only pins, which is why they're used for
the analog and digital rain lines (input-only is fine for both).

Pins are configurable in `include/config.h`:
- `SDA_PIN` / `SCL_PIN` — BMP280 I2C
- `ONE_WIRE_PIN` — DS18B20
- `DHT_PIN` — DHT11
- `RAIN_ANALOG_PIN` / `RAIN_DIGITAL_PIN` — rain sensor

The BMP280 is typically at I2C address `0x76` or `0x77` — the firmware tries
both, but set `BMP280_ADDRESS` in `config.h` if you know yours.

### Calibrating the rain sensor

The analog reading is converted to a 0-100% intensity using `RAIN_ADC_DRY`
and `RAIN_ADC_WET` in `config.h`. To calibrate: flash the firmware, open the
serial monitor, note the raw ADC value with the board bone dry (should be
close to 4095) and again with a few drops of water on it, then update those
two constants.

## Software setup

### Option A: PlatformIO

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

This first flash has to happen over USB. After that, the firmware has
WiFi OTA (over-the-air) updates built in — see below.

### Option B: Arduino IDE

The Arduino IDE uses the same Arduino framework as this project, but it only
compiles files that sit directly in the sketch folder (no `src/`/`include/`
subfolders) and the sketch's main file must share the folder's name. To use
it:

1. Install the ESP32 board package: **Tools → Board → Boards Manager**,
   search "esp32", install the one by Espressif Systems. (If it's not
   listed, add `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   under **File → Preferences → Additional Boards Manager URLs** first.)
2. Install the libraries below via **Sketch → Include Library → Manage
   Library** (search each name, install the listed author's version — these
   are the same dependencies as `platformio.ini`):
   - Adafruit BMP280 Library (Adafruit)
   - Adafruit Unified Sensor (Adafruit)
   - DallasTemperature (Miles Burton)
   - OneWire (Paul Stoffregen)
   - DHT sensor library (Adafruit)
   - ESPAsyncWebServer (ESP32Async)
   - AsyncTCP (ESP32Async)
   - ArduinoJson (Benoit Blanchon)
3. Create a sketch folder named `WeatherWebServer` and copy these files into
   it flat (no subfolders):
   - `src/main.cpp` → `WeatherWebServer/WeatherWebServer.ino`
   - `src/weather_sensor.cpp` → `WeatherWebServer/weather_sensor.cpp`
   - `include/weather_sensor.h` → `WeatherWebServer/weather_sensor.h`
   - `include/config.h` → `WeatherWebServer/config.h`
   - `include/secrets.h.example` → `WeatherWebServer/secrets.h`, then fill in
     your WiFi credentials
4. Open `WeatherWebServer.ino` in the Arduino IDE, select your board and
   port under **Tools**, and upload.
5. Upload the dashboard (`data/index.html`) to the ESP32's filesystem. The
   Arduino IDE doesn't do this out of the box:
   - **Arduino IDE 1.8.x**: install the
     [ESP32 Sketch Data Upload](https://github.com/me-no-dev/arduino-esp32fs-plugin)
     plugin, put `index.html` in a `data/` folder next to the `.ino`, then
     use **Tools → ESP32 Sketch Data Upload**.
   - **Arduino IDE 2.x**: that plugin isn't supported. Easiest path is to
     install PlatformIO just for this one step (`pio run --target uploadfs`
     from this repo, using its `data/` folder) — the sketch itself can still
     be built/flashed from the Arduino IDE day-to-day.
6. Open the serial monitor (**Tools → Serial Monitor**, 115200 baud) to find
   the assigned IP address, or visit `http://weather.local`.

Since Arduino IDE compiles a flat copy of the source, keep the PlatformIO
`src/`/`include/` files as the source of truth and re-copy after changes —
editing both trees independently will drift.

## Updating over WiFi (OTA)

Once the device is running (flashed at least once over USB), you can push
new firmware and dashboard updates over WiFi instead of plugging back in.
The firmware requires a password for this (`OTA_PASSWORD` in `secrets.h`) so
random devices on your network can't push code to it.

With PlatformIO:

```sh
OTA_HOST=weather.local OTA_PASSWORD=yourpassword pio run -e esp32dev_ota -t upload
OTA_HOST=weather.local OTA_PASSWORD=yourpassword pio run -e esp32dev_ota -t uploadfs
```

`OTA_HOST` can be the mDNS hostname (`weather.local`, or whatever you set
`MDNS_HOSTNAME` to) or the device's IP address if mDNS resolution isn't
working on your machine. `OTA_PASSWORD` must match the value baked into the
firmware via `secrets.h` — set both as shell environment variables (not in
`platformio.ini`) so the password never ends up committed to git.

If you're using the Arduino IDE instead: once the board shows up under
**Tools → Port** as a network port (it appears automatically over mDNS once
OTA-enabled firmware is running), select it there and upload as normal — the
IDE will prompt for the OTA password.

## What it does

- Reads all four sensors every 5 seconds (`SENSOR_READ_INTERVAL_MS` in
  `config.h`). Temperature comes from the DS18B20, humidity from the DHT11,
  pressure/altitude from the BMP280, and rain intensity/detection from the
  rain sensor — each tracked independently so one sensor failing doesn't
  blank out the others.
- Keeps a rolling 24-hour history in memory (sampled every 5 minutes),
  used for the on-device forecast calculation below.
- Serves a dashboard at `/` with live temperature, humidity, pressure,
  altitude, rain intensity, a short-term forecast, a radar map, and trend
  charts for temperature/humidity/pressure (each on its own scale, since
  they don't share units) with a selectable time range from 1 hour to all
  time, auto-refreshing every 5 seconds. Any sensor that isn't detected
  shows `--` and is called out in the status line; the status line also
  flags when it's currently raining.
- Computes a simple pressure-trend forecast on-device (no internet needed):
  it compares the current pressure to the reading from `FORECAST_LOOKBACK_MS`
  ago (3 hours by default) and reports whether that trend points toward
  improving or worsening weather. This is a rough heuristic, not a real
  forecast model — it needs a few hours of history after boot before it has
  enough data to say anything.
- Embeds a live radar map in the dashboard using
  [RainViewer](https://www.rainviewer.com/)'s free, keyless embed — this is
  fetched directly by your browser, not the ESP32, so it needs your station's
  coordinates set (`STATION_LAT`/`STATION_LON` in `secrets.h`, see below) and
  internet access on whatever device you're viewing the dashboard from.
- Exposes JSON APIs:
  - `GET /api/current` — latest reading, with `temp_valid` / `humidity_valid`
    / `pressure_valid` / `rain_valid` flags per sensor, plus
    `rain_intensity_pct`, `is_raining`, `forecast`, `forecast_trend_valid`,
    and (once valid) `forecast_trend_hpa_3h`
  - `GET /api/history` — last 24h from the in-memory buffer (used
    internally for the forecast; not used by the dashboard's charts)
  - `GET /api/history/range?range=1h|6h|12h|1d|1w|1mo|1y|all` — trend chart
    data proxied from InfluxDB (see below), downsampled per range to a
    manageable number of points
  - `GET /api/config` — non-secret config the dashboard needs at runtime
    (currently just `station_lat`/`station_lon` for the radar map), kept
    off of `data/index.html` since that file is committed to git

## Long-term history (InfluxDB) — currently disabled in the dashboard

**Status:** the dashboard's Trends section is temporarily back to showing
only the last 24h (from the ESP32's local RAM buffer), with the range
selector capped at 1 day. The InfluxDB write path and the
`/api/history/range` endpoint described below are still in the firmware and
still running — they're just not wired into the dashboard UI right now
while the InfluxDB connection is debugged (writes/queries were returning
non-200s in testing). To pick this back up: swap `data/index.html`'s
`pollHistory()` back to fetching `/api/history/range?range=...` instead of
filtering the local `/api/history` buffer, and restore the longer options
in `#range-select`.

The 24h RAM buffer above can't hold a week/month/year of history — the ESP32
doesn't have the memory for it, and it resets on every reboot anyway. For the
dashboard's Trends time-range selector to show anything beyond 24h, the
firmware pushes every 5-minute sample to a self-hosted
[InfluxDB](https://www.influxdata.com/) 2.x instance, and the dashboard's
time range selector reads it back through the ESP32 (which proxies the
query so your InfluxDB API token never reaches the browser).

**Setup:**

1. Run InfluxDB 2.x somewhere reachable from your ESP32 (a Raspberry Pi,
   home server, or Docker container — see InfluxDB's own docs for that
   part, it's outside the scope of this firmware).
2. In InfluxDB, create an organization, a bucket (e.g. `weather`), and an
   API token with read+write access to that bucket.
3. Fill in `include/secrets.h`:
   ```
   #define INFLUXDB_URL "http://192.168.1.50:8086"
   #define INFLUXDB_ORG "your-org"
   #define INFLUXDB_BUCKET "weather"
   #define INFLUXDB_TOKEN "your-api-token"
   ```
4. Flash/OTA-update the firmware. Readings start appearing in InfluxDB
   within a few minutes (measurement `weather`, fields `temperature_c`,
   `humidity_pct`, `pressure_hpa`, `rain_intensity_pct`, `is_raining`).

**Notes:**

- If InfluxDB is unreachable, sample writes just fail silently (logged over
  serial) — the rest of the firmware keeps working normally, and the
  dashboard's Trends section shows a "could not reach InfluxDB" notice
  instead of breaking.
- Each 5-minute write and each `/api/history/range` request is a *blocking*
  HTTP call on the ESP32 (bounded by a few seconds' timeout), since this
  firmware has no async HTTP client. Fine for a single-station LAN
  dashboard; don't expect it to hold up under heavy concurrent load.
- Timestamps use InfluxDB's own write-time, not an ESP32 clock — the ESP32
  doesn't sync time via NTP, so this avoids needing that.
- "All time" is implemented as a 100-year lookback rather than a true
  unbounded query (Flux's `range()` wants a duration, not an open-ended
  start) — in practice this is indistinguishable from "everything."

## Project layout

```
include/
  config.h            Sensor pins, addresses, and timing constants
  secrets.h.example    Template for WiFi/OTA/InfluxDB credentials (copy to secrets.h)
  weather_sensor.h      Sensor wrapper interface (BMP280 + DS18B20 + DHT11 + rain)
  influx_client.h       InfluxDB write/query interface
src/
  main.cpp             WiFi, web server, REST API, history buffer, OTA
  weather_sensor.cpp    Per-sensor read logic
  influx_client.cpp     InfluxDB line-protocol writes and Flux queries
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
- **Radar location**: set `STATION_LAT` / `STATION_LON` in
  `include/secrets.h` to your coordinates, then reflash the firmware
  (`pio run --target upload` or the OTA equivalent) — the dashboard fetches
  them from the device via `GET /api/config` rather than having them
  hardcoded in `data/index.html`, since that file is committed to git. Left
  at `0, 0` the radar section shows a setup reminder instead of a map.
- **Forecast sensitivity**: `FORECAST_LOOKBACK_MS` in `config.h` controls how
  far back the pressure trend looks (3 hours by default). Shorter windows
  react faster but are noisier; longer windows are smoother but slower to
  pick up on changes.
