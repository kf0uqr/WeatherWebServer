#pragma once

#include <ArduinoJson.h>
#include "weather_sensor.h"

// Pushes one reading to InfluxDB (fire-and-forget; failures are logged but
// never block the rest of the firmware). Only fields with valid readings
// are written. No local storage of this data - InfluxDB is the source of
// truth for anything beyond the on-device 24h RAM history.
void influxWrite(const WeatherReading &reading);

// Queries one field's mean, downsampled to `every`, over the last `rangeFlux`
// (a Flux duration/range expression, e.g. "-6h" or "0" for all time), and
// appends {"t": <RFC3339 string>, "v": <float>} objects to outArray.
// Returns false (leaving outArray empty or partial) on any network/parse
// failure - callers should treat that as "no data available right now"
// rather than a fatal error.
bool influxQueryField(const char *field, const char *rangeFlux, const char *every, JsonArray &outArray);
