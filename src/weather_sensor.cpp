#include "weather_sensor.h"
#include "config.h"
#include <Wire.h>

bool WeatherSensor::begin() {
    Wire.begin(SDA_PIN, SCL_PIN);
    ready = bme.begin(BME280_ADDRESS);
    if (!ready) {
        // Some breakout boards ship with the sensor strapped to the other address.
        ready = bme.begin(BME280_ADDRESS == 0x76 ? 0x77 : 0x76);
    }
    return ready;
}

WeatherReading WeatherSensor::read() {
    WeatherReading reading{};
    reading.valid = ready;
    if (!ready) {
        return reading;
    }

    reading.temperatureC = bme.readTemperature();
    reading.humidityPct = bme.readHumidity();
    reading.pressureHpa = bme.readPressure() / 100.0F;
    reading.altitudeM = bme.readAltitude(SEA_LEVEL_PRESSURE_HPA);
    return reading;
}
