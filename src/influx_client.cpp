#include "influx_client.h"
#include <HTTPClient.h>
#include <WiFi.h>

#if __has_include("secrets.h")
#include "secrets.h"
#endif

static void splitCsvLine(const String &line, String tokens[], int maxTokens, int &count) {
    count = 0;
    int start = 0;
    while (count < maxTokens) {
        int comma = line.indexOf(',', start);
        if (comma < 0) {
            tokens[count++] = line.substring(start);
            break;
        }
        tokens[count++] = line.substring(start, comma);
        start = comma + 1;
    }
}

void influxWrite(const WeatherReading &reading) {
    if (WiFi.status() != WL_CONNECTED) return;

    String fields;
    if (reading.tempValid) fields += "temperature_c=" + String(reading.temperatureC, 2) + ",";
    if (reading.humidityValid) fields += "humidity_pct=" + String(reading.humidityPct, 2) + ",";
    if (reading.pressureValid) fields += "pressure_hpa=" + String(reading.pressureHpa, 2) + ",";
    if (reading.rainValid) {
        fields += "rain_intensity_pct=" + String(reading.rainIntensityPct, 2) + ",";
        fields += "is_raining=" + String(reading.isRaining ? 1 : 0) + "i,";
    }

    if (fields.isEmpty()) return; // nothing valid to report
    fields.remove(fields.length() - 1); // drop trailing comma

    // No explicit timestamp - InfluxDB stamps it with server-side arrival
    // time, which is accurate enough given we write every few minutes and
    // avoids needing NTP time sync on the ESP32.
    String line = "weather,station=esp32 " + fields;

    HTTPClient http;
    String url = String(INFLUXDB_URL) + "/api/v2/write?org=" + INFLUXDB_ORG +
                 "&bucket=" + INFLUXDB_BUCKET + "&precision=s";
    http.begin(url);
    http.setTimeout(3000); // bound the blocking call - this runs in the main loop
    http.addHeader("Authorization", String("Token ") + INFLUXDB_TOKEN);
    http.addHeader("Content-Type", "text/plain; charset=utf-8");

    int code = http.POST(line);
    if (code != 204) {
        Serial.printf("InfluxDB write failed: HTTP %d\n", code);
    }
    http.end();
}

bool influxQueryField(const char *field, const char *rangeFlux, const char *every, JsonArray &outArray) {
    if (WiFi.status() != WL_CONNECTED) return false;

    String flux = "from(bucket: \"" + String(INFLUXDB_BUCKET) + "\")\n"
                  "  |> range(start: " + rangeFlux + ")\n"
                  "  |> filter(fn: (r) => r._measurement == \"weather\" and r._field == \"" + field + "\")\n"
                  "  |> aggregateWindow(every: " + every + ", fn: mean, createEmpty: false)\n"
                  "  |> yield(name: \"mean\")";

    HTTPClient http;
    String url = String(INFLUXDB_URL) + "/api/v2/query?org=" + INFLUXDB_ORG;
    http.begin(url);
    http.setTimeout(5000); // this runs inside an HTTP request handler - keep it bounded
    http.addHeader("Authorization", String("Token ") + INFLUXDB_TOKEN);
    http.addHeader("Content-Type", "application/vnd.flux");
    http.addHeader("Accept", "application/csv");

    int code = http.POST(flux);
    if (code != 200) {
        Serial.printf("InfluxDB query failed for %s: HTTP %d\n", field, code);
        http.end();
        return false;
    }

    String body = http.getString();
    http.end();

    const int MAX_COLS = 16;
    String tokens[MAX_COLS];
    int tokenCount = 0;

    int timeCol = -1, valueCol = -1;
    bool haveHeader = false;

    // Flux's annotated CSV groups each table as: blank line, "#"-prefixed
    // annotation rows, a header row, then data rows. A blank line always
    // means "the next header row starts a new table" - that's the only
    // reliable boundary marker, so re-arm on every blank line rather than
    // trying to infer it from column layout.
    int pos = 0;
    while (pos < (int)body.length()) {
        int nl = body.indexOf('\n', pos);
        String line = (nl < 0) ? body.substring(pos) : body.substring(pos, nl);
        pos = (nl < 0) ? body.length() : nl + 1;

        line.trim();
        if (line.isEmpty()) {
            haveHeader = false;
            continue;
        }
        if (line.startsWith("#")) {
            continue;
        }

        splitCsvLine(line, tokens, MAX_COLS, tokenCount);

        if (!haveHeader) {
            timeCol = valueCol = -1;
            for (int i = 0; i < tokenCount; i++) {
                if (tokens[i] == "_time") timeCol = i;
                if (tokens[i] == "_value") valueCol = i;
            }
            haveHeader = true;
            continue;
        }

        if (timeCol < 0 || valueCol < 0 || timeCol >= tokenCount || valueCol >= tokenCount) {
            continue; // malformed row - skip rather than record garbage
        }

        JsonObject point = outArray.add<JsonObject>();
        point["t"] = tokens[timeCol];
        point["v"] = tokens[valueCol].toFloat();
    }

    return true;
}
