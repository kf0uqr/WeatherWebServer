#pragma once

#include <Adafruit_BME280.h>

struct WeatherReading {
    float temperatureC;
    float humidityPct;
    float pressureHpa;
    float altitudeM;
    bool valid;
};

class WeatherSensor {
public:
    bool begin();
    WeatherReading read();

private:
    Adafruit_BME280 bme;
    bool ready = false;
};
