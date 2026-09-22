#pragma once

#include <Adafruit_BMP280.h>
#include <DHT.h>
#include <DallasTemperature.h>
#include <OneWire.h>

#include "config.h"

struct WeatherReading {
    float temperatureC;
    float humidityPct;
    float pressureHpa;
    float altitudeM;
    float rainIntensityPct; // 0 = dry, 100 = fully wet
    bool isRaining;         // digital threshold from the sensor board
    bool tempValid;
    bool humidityValid;
    bool pressureValid;
    bool rainValid;
};

class WeatherSensor {
public:
    bool begin();
    WeatherReading read();

private:
    Adafruit_BMP280 bmp;
    OneWire oneWire{ONE_WIRE_PIN};
    DallasTemperature ds18b20{&oneWire};
    DHT dht{DHT_PIN, DHT11};

    bool bmpReady = false;
    bool ds18b20Ready = false;
};
