#include <ArduinoJson.h>
#include <RadioLib.h>
#include <config.h>

String PrintChannel(int channel, JsonDocument base)
{
    JsonObject obj = base[channel - 1];
    float tfreq = obj["freq"];
    float tOffset = obj["of"];
    int tSF = obj["sf"];
    float tBW = obj["bw"];

    String conf = "Set Channel : ";
    conf += String(channel) + " ";
    conf += "@rx = " + String(tfreq, 3) + " ";
    conf += "@of = " + String(tOffset, 3) + " ";
    conf += "@bw = " + String(tBW) + " ";   
    conf += "@sf = " + String(tSF) + " "; 
    Serial.println(conf);
    return conf;    
}

String ListChannel(JsonDocument base)
{
    String conf;
    for (int i = 0; i < base.size(); i++) {

    JsonObject obj = base[i];
    float tfreq = obj["freq"];
    float tOffset = obj["of"];
    int tSF = obj["sf"];
    float tBW = obj["bw"];

    conf += "Ch : ";
    conf += String(i+1) + " ";
    conf += "@rx = " + String(tfreq, 3) + " ";
    conf += "@of = " + String(tOffset, 3) + " ";
    conf += "@bw = " + String(tBW) + " ";   
    conf += "@sf = " + String(tSF) + "\n";
    } 
    Serial.println(conf);
    return conf;    
}

void SetChannel(int channel, JsonDocument base, SX1262 radio)
{
    JsonObject obj = base[channel - 1];
    RX_freq = obj["freq"];
    Offset = obj["of"];
    SF = obj["sf"];
    BW = obj["bw"];

    radio.setFrequency(RX_freq + Freq_error + Offset);
    radio.setBandwidth(BW);
    radio.setSpreadingFactor(SF);
}
