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

// Rain drop sensor (FC-37 / YL-83 style board): analog pin for intensity,
// digital pin for the onboard comparator's rain/no-rain threshold.
// The analog pin must be an ADC-capable GPIO (32-39 are ADC1, safe to use
// alongside WiFi; avoid ADC2 pins like 0/2/4/12-15/25-27 since WiFi disables them).
#define RAIN_ANALOG_PIN 34
#define RAIN_DIGITAL_PIN 35

// Raw ADC readings (0-4095) for a bone-dry board and a fully wet board.
// Calibrate these for your board: read raw values dry and under a few drops
// of water, then update here. Values fall as the board gets wetter.
#define RAIN_ADC_DRY 4095
#define RAIN_ADC_WET 1500

// How often to read the sensors, in milliseconds.
#define SENSOR_READ_INTERVAL_MS 5000

// How many samples to keep for the in-memory history graph.
#define HISTORY_SIZE 288 // 24h of history at a 5 minute sample interval
#define HISTORY_SAMPLE_INTERVAL_MS 300000

// Sea-level pressure at your location, used to compute altitude.
// Adjust to your local station pressure (hPa) for a more accurate reading.
#define SEA_LEVEL_PRESSURE_HPA 1013.25
