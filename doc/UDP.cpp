#include "WiFi.h"
#include "AsyncUDP.h"

const char * ssid = "cv_home";
const char * password = "naksitral";

AsyncUDP udp;

void setup(){
    pinMode(2, OUTPUT); // HERE LED2
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("WiFi Failed");
        while(1) {
            delay(1000);
        }
    }
    if(udp.listen(7777)) {
        Serial.print("UDP Listening on IP: ");
        Serial.println(WiFi.localIP());
        udp.onPacket([](AsyncUDPPacket packet) {
            Serial.print("UDP Packet Type: ");
            Serial.print(packet.isBroadcast()?"Broadcast":packet.isMulticast()?"Multicast":"Unicast");
            Serial.print(", From: ");
            Serial.print(packet.remoteIP());
            Serial.print(":");
            Serial.print(packet.remotePort());
            Serial.print(", To: ");
            Serial.print(packet.localIP());
            Serial.print(":");
            Serial.print(packet.localPort());
            Serial.print(", Length: ");
            Serial.print(packet.length());
            Serial.print(", Data: ");
            Serial.write(packet.data(), packet.length());
            Serial.println();

// https://stackoverflow.com/questions/58253174/saving-packet-data-to-string-with-asyncudp-on-esp32
             // String  mensaje = (char*)(packet.data());
            char* tmpStr = (char*) malloc(packet.length() + 1);
            memcpy(tmpStr, packet.data(), packet.length());
            tmpStr[packet.length()] = '\0'; // ensure null termination
            String mensaje = String(tmpStr);
            free(tmpStr); // Strign(char*) creates a copy so we can delete our one

            Serial.println(mensaje);
            if(mensaje == "on"){
              digitalWrite(2, HIGH);
              packet.print("Received: on");
            }
            if(mensaje == "off"){
              digitalWrite(2, LOW);
              packet.print("Received: off");
            }
            if(mensaje == "consulta"){
              if(digitalRead(2 == HIGH)){packet.print("Now LED2 is HIGH");}
              if(digitalRead(2 == LOW)) {packet.print("Now LED2 is LOW");}
            }

            //reply to the client
            //packet.printf("Got %u bytes of data", packet.length());
        });
    }
}

void loop(){
    delay(1000);
    //Send broadcast
    udp.broadcast("Anyone here?");
}