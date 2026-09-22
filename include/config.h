#pragma once

// I2C pins for the BME280 (defaults match most ESP32 dev boards).
#define SDA_PIN 21
#define SCL_PIN 22

// BME280 is available at 0x76 or 0x77 depending on the breakout board.
#define BME280_ADDRESS 0x76

// How often to read the sensor, in milliseconds.
#define SENSOR_READ_INTERVAL_MS 5000

// How many samples to keep for the in-memory history graph.
#define HISTORY_SIZE 288 // 24h of history at a 5 minute sample interval
#define HISTORY_SAMPLE_INTERVAL_MS 300000

// Sea-level pressure at your location, used to compute altitude.
// Adjust to your local station pressure (hPa) for a more accurate reading.
#define SEA_LEVEL_PRESSURE_HPA 1013.25
