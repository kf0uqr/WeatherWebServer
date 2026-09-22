#pragma once

// I2C pins for the BMP280 barometer (defaults match most ESP32 dev boards).
#define SDA_PIN 21
#define SCL_PIN 22

// BMP280 is available at 0x76 or 0x77 depending on the breakout board.
#define BMP280_ADDRESS 0x76

// OneWire data pin for the DS18B20 temperature probe.
#define ONE_WIRE_PIN 4

// Digital data pin for the DHT11 humidity sensor.
#define DHT_PIN 27

// How often to read the sensors, in milliseconds.
#define SENSOR_READ_INTERVAL_MS 5000

// How many samples to keep for the in-memory history graph.
#define HISTORY_SIZE 288 // 24h of history at a 5 minute sample interval
#define HISTORY_SAMPLE_INTERVAL_MS 300000

// Sea-level pressure at your location, used to compute altitude.
// Adjust to your local station pressure (hPa) for a more accurate reading.
#define SEA_LEVEL_PRESSURE_HPA 1013.25
