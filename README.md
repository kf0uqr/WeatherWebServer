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

- Reads all four sensors every 5 seconds (`SENSOR_READ_INTERVAL_MS` in
  `config.h`). Temperature comes from the DS18B20, humidity from the DHT11,
  pressure/altitude from the BMP280, and rain intensity/detection from the
  rain sensor — each tracked independently so one sensor failing doesn't
  blank out the others.
- Keeps a rolling 24-hour history in memory (sampled every 5 minutes) for the
  temperature and rain trend data.
- Serves a dashboard at `/` with live temperature, humidity, pressure,
  altitude, and rain intensity, auto-refreshing every 5 seconds. Any sensor
  that isn't detected shows `--` and is called out in the status line; the
  status line also flags when it's currently raining.
- Exposes JSON APIs:
  - `GET /api/current` — latest reading, with `temp_valid` / `humidity_valid`
    / `pressure_valid` / `rain_valid` flags per sensor, plus
    `rain_intensity_pct` and `is_raining`
  - `GET /api/history` — recent history points for charting

## Project layout

```
include/
  config.h            Sensor pins, addresses, and timing constants
  secrets.h.example    Template for WiFi credentials (copy to secrets.h)
  weather_sensor.h      Sensor wrapper interface (BMP280 + DS18B20 + DHT11 + rain)
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
