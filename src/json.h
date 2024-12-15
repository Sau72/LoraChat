#include <ArduinoJson.h>
#include <RadioLib.h>

String PrintChannel(int channel, JsonDocument base);
void SetChannel(int channel, JsonDocument base, SX1262 radio);
String ListChannel(JsonDocument base);