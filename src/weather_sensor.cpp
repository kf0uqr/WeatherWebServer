#include "weather_sensor.h"
#include <Wire.h>

bool WeatherSensor::begin() {
    Wire.begin(SDA_PIN, SCL_PIN);
    bmpReady = bmp.begin(BMP280_ADDRESS);
    if (!bmpReady) {
        // Some breakout boards ship with the sensor strapped to the other address.
        bmpReady = bmp.begin(BMP280_ADDRESS == 0x76 ? 0x77 : 0x76);
    }

    ds18b20.begin();
    ds18b20Ready = ds18b20.getDeviceCount() > 0;

    dht.begin();

    pinMode(RAIN_DIGITAL_PIN, INPUT);

    return bmpReady || ds18b20Ready;
}

WeatherReading WeatherSensor::read() {
    WeatherReading reading{};

    if (ds18b20Ready) {
        ds18b20.requestTemperatures();
        float t = ds18b20.getTempCByIndex(0);
        reading.tempValid = (t != DEVICE_DISCONNECTED_C);
        reading.temperatureC = t;
    }

    float h = dht.readHumidity();
    reading.humidityValid = !isnan(h);
    reading.humidityPct = h;

    if (bmpReady) {
        reading.pressureHpa = bmp.readPressure() / 100.0F;
        reading.altitudeM = bmp.readAltitude(SEA_LEVEL_PRESSURE_HPA);
        reading.pressureValid = true;
    }

    int rainRaw = analogRead(RAIN_ANALOG_PIN);
    float pct = (float)(RAIN_ADC_DRY - rainRaw) / (float)(RAIN_ADC_DRY - RAIN_ADC_WET) * 100.0F;
    reading.rainIntensityPct = constrain(pct, 0.0F, 100.0F);
    reading.isRaining = (digitalRead(RAIN_DIGITAL_PIN) == LOW); // board pulls DO low when wet
    reading.rainValid = true;

    return reading;
}
