#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#if __has_include("secrets.h")
#include "secrets.h"
#else
#error "Missing include/secrets.h - copy include/secrets.h.example to include/secrets.h and fill in your WiFi credentials."
#endif

#include "config.h"
#include "weather_sensor.h"

WeatherSensor sensor;
AsyncWebServer server(80);

WeatherReading latest{};
unsigned long lastReadMs = 0;

struct HistoryPoint {
    unsigned long timestampMs;
    float temperatureC;
    float humidityPct;
    float pressureHpa;
    float rainIntensityPct;
    bool pressureValid;
};

HistoryPoint history[HISTORY_SIZE];
size_t historyCount = 0;
size_t historyHead = 0; // index of the oldest sample
unsigned long lastHistoryMs = 0;

void pushHistory(const WeatherReading &reading) {
    size_t writeIndex = (historyHead + historyCount) % HISTORY_SIZE;
    history[writeIndex] = {millis(), reading.temperatureC, reading.humidityPct, reading.pressureHpa,
                            reading.rainIntensityPct, reading.pressureValid};

    if (historyCount < HISTORY_SIZE) {
        historyCount++;
    } else {
        historyHead = (historyHead + 1) % HISTORY_SIZE;
    }
}

// Samples spanning the lookback window used for the pressure-trend forecast.
const size_t FORECAST_LOOKBACK_SAMPLES = FORECAST_LOOKBACK_MS / HISTORY_SAMPLE_INTERVAL_MS;

// A simple pressure-trend heuristic, not a substitute for a real forecast:
// it just says whether pressure has been rising or falling over the last
// few hours, which correlates loosely with improving/worsening weather.
const char *pressureTrendForecast(float &trendHpaOut, bool &validOut) {
    validOut = false;
    trendHpaOut = 0;

    if (!latest.pressureValid || historyCount <= FORECAST_LOOKBACK_SAMPLES) {
        return "Gathering data...";
    }

    size_t newestIdx = (historyHead + historyCount - 1) % HISTORY_SIZE;
    size_t pastIdx = (historyHead + historyCount - 1 - FORECAST_LOOKBACK_SAMPLES) % HISTORY_SIZE;

    if (!history[newestIdx].pressureValid || !history[pastIdx].pressureValid) {
        return "Gathering data...";
    }

    float trend = history[newestIdx].pressureHpa - history[pastIdx].pressureHpa;
    trendHpaOut = trend;
    validOut = true;

    if (trend <= -6.0F) return "Stormy - unsettled weather likely";
    if (trend <= -3.6F) return "Rain likely, unsettled";
    if (trend <= -1.6F) return "Cloudy, chance of rain";
    if (trend < 1.6F) return "Steady - no big change expected";
    if (trend < 3.6F) return "Improving, becoming fair";
    return "Fair - settled weather likely";
}

void connectWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.printf("Connecting to WiFi \"%s\"", WIFI_SSID);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        if (millis() - start > 30000) {
            Serial.println("\nWiFi connection timed out, retrying...");
            WiFi.disconnect();
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
            start = millis();
        }
    }

    Serial.printf("\nConnected! IP address: %s\n", WiFi.localIP().toString().c_str());
}

String currentReadingJson() {
    JsonDocument doc;
    doc["temp_valid"] = latest.tempValid;
    doc["humidity_valid"] = latest.humidityValid;
    doc["pressure_valid"] = latest.pressureValid;
    doc["rain_valid"] = latest.rainValid;
    doc["temperature_c"] = latest.temperatureC;
    doc["temperature_f"] = latest.temperatureC * 9.0F / 5.0F + 32.0F;
    doc["humidity_pct"] = latest.humidityPct;
    doc["pressure_hpa"] = latest.pressureHpa;
    doc["altitude_m"] = latest.altitudeM;
    doc["rain_intensity_pct"] = latest.rainIntensityPct;
    doc["is_raining"] = latest.isRaining;

    float trendHpa;
    bool trendValid;
    doc["forecast"] = pressureTrendForecast(trendHpa, trendValid);
    doc["forecast_trend_valid"] = trendValid;
    if (trendValid) {
        doc["forecast_trend_hpa_3h"] = trendHpa;
    }

    doc["uptime_ms"] = millis();

    String out;
    serializeJson(doc, out);
    return out;
}

String historyJson() {
    JsonDocument doc;
    JsonArray points = doc["points"].to<JsonArray>();

    for (size_t i = 0; i < historyCount; i++) {
        size_t idx = (historyHead + i) % HISTORY_SIZE;
        JsonObject point = points.add<JsonObject>();
        point["t"] = history[idx].timestampMs;
        point["temperature_c"] = history[idx].temperatureC;
        point["humidity_pct"] = history[idx].humidityPct;
        point["pressure_hpa"] = history[idx].pressureHpa;
        point["rain_intensity_pct"] = history[idx].rainIntensityPct;
    }

    String out;
    serializeJson(doc, out);
    return out;
}

void setupOTA() {
    ArduinoOTA.setHostname(MDNS_HOSTNAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);

    ArduinoOTA.onStart([]() {
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "firmware" : "filesystem";
        Serial.println("OTA update starting: " + type);
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\nOTA update complete, rebooting...");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("OTA progress: %u%%\r", (progress * 100) / total);
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("OTA error [%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("auth failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("begin failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("connect failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("receive failed");
        else if (error == OTA_END_ERROR) Serial.println("end failed");
    });

    ArduinoOTA.begin();
    Serial.println("OTA ready");
}

void setupServer() {
    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

    server.on("/api/current", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "application/json", currentReadingJson());
    });

    server.on("/api/history", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "application/json", historyJson());
    });

    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Not found");
    });

    server.begin();
}

void setup() {
    Serial.begin(115200);
    delay(200);

    if (!SPIFFS.begin(true)) {
        Serial.println("Failed to mount SPIFFS. Did you run 'pio run --target uploadfs'?");
    }

    if (!sensor.begin()) {
        Serial.println("No sensors detected. Check wiring and pins in include/config.h.");
    }

    connectWiFi();

    if (MDNS.begin(MDNS_HOSTNAME)) {
        MDNS.addService("http", "tcp", 80);
        Serial.printf("mDNS ready at http://%s.local\n", MDNS_HOSTNAME);
    } else {
        Serial.println("mDNS setup failed.");
    }

    setupOTA();
    setupServer();

    latest = sensor.read();
    pushHistory(latest);
    lastReadMs = millis();
    lastHistoryMs = millis();
}

void loop() {
    unsigned long now = millis();

    if (now - lastReadMs >= SENSOR_READ_INTERVAL_MS) {
        lastReadMs = now;
        latest = sensor.read();
    }

    if (now - lastHistoryMs >= HISTORY_SAMPLE_INTERVAL_MS) {
        lastHistoryMs = now;
        pushHistory(latest);
    }

    if (WiFi.status() != WL_CONNECTED) {
        connectWiFi();
    }

    ArduinoOTA.handle();
}
