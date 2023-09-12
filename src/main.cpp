/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-webserial-library/
  
  This sketch is based on the WebSerial library example: ESP32_Demo
  https://github.com/ayushsharma82/WebSerial
*/

#include <Arduino.h>
#include <WiFi.h>
#include "AsyncUDP.h"


#define LED 2

//UDP port
const uint16_t port = 7777;

//Access point or client
//#define WF_AP

// include the library
#include <RadioLib.h>

// SX1262 has the following connections:
// NSS pin:   10
// DIO1 pin:  2
// NRST pin:  3
// BUSY pin:  9
//SX1262 radio1 = new Module(10, 2, 3, 9);

float RX_freq = 262.225;
float TX_freq = 262.225+33.6;

SX1262 radio = new Module(5, 21, 33, 26);

AsyncUDP udp;

#ifdef WF_AP
  const char* ssid = "LoraSat";          // Your WiFi SSID
#else
  const char* ssid = "cv_home";          // Your WiFi SSID
#endif
const char* password = "naksitral";  // Your WiFi Password

// save transmission states between loops
int transmissionState = RADIOLIB_ERR_NONE;

// flag to indicate transmission or reception state
bool transmitFlag = false;

// flag to indicate that a packet was sent or received
volatile bool operationDone = false;

// this function is called when a complete packet
// is transmitted or received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
ICACHE_RAM_ATTR
void setFlag(void) {
  // we sent or received a packet, set the flag
  operationDone = true;
}



void transmit(String &data){
  // turn LED on
  digitalWrite(2, HIGH);  // turn the LED on (HIGH is the voltage level)
  
  Serial.println(F("[SX1262] Transmitting packet ... "));
  //int state = radio.startTransmit(data);
  radio.setFrequency(TX_freq);
  transmissionState = radio.startTransmit(data);
  transmitFlag = true;
}

void parsePacket(AsyncUDPPacket packet)
{

  Serial.print("UDP Packet From: ");
  Serial.print(packet.remoteIP());
  Serial.print(":");
  Serial.print(packet.remotePort());
  Serial.print(", Data: ");
  Serial.write(packet.data(), packet.length());
  Serial.println();

  // https://stackoverflow.com/questions/58253174/saving-packet-data-to-string-with-asyncudp-on-esp32
  // String  mensaje = (char*)(packet.data());
  char* tmpStr = (char*) malloc(packet.length() + 1);
  memcpy(tmpStr, packet.data(), packet.length());
  tmpStr[packet.length()] = '\0'; // ensure null termination
  String message = String(tmpStr);
  free(tmpStr); // Strign(char*) creates a copy so we can delete our one

  Serial.println(message);
  if(message == "on"){
    digitalWrite(2, HIGH);
    packet.print("Received: on");
    }
  if(message == "off"){
    digitalWrite(2, LOW);
    packet.print("Received: off");
    }

  //send to lora
  transmit(message);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  
  // Connect to Wi-Fi network with SSID and password
  Serial.print("[WiFi] Initializing...");
  // Remove the password parameter, if you want the AP (Access Point) to be open

  #ifdef WF_AP
    WiFi.softAP(ssid, password);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
  #else
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.printf("WiFi Failed!\n");
      return;
    }
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  #endif
  
  
  if(udp.listen(port)) {
        // При получении пакета вызываем callback функцию
        udp.onPacket(parsePacket);
    }

  // initialize SX1262 with default settings
  Serial.print(F("[SX1262] Initializing ... "));

  //int state = radio.begin(262.225+33.6, 15.63, 9, 5, 0x34, 22, 8, 0); // works
  //int state = radio.begin(262.226+33.6, 20.83, 9, 5, 0x34, 22, 8, 0); // works
  int state = radio.begin(RX_freq, 20.83, 9, 5, 0x34, 22, 8, 0); // works
  //radio.setTCXO(3.3, 1563);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  radio.setPacketSentAction(setFlag);
  radio.setPacketReceivedAction(setFlag);
  //radio.setDio1Action(setFlag);

  Serial.print(F("[SX1262] Starting to listen ... "));
  state = radio.startReceive();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }
}

void loop() {  
// check if the previous operation finished
  if(operationDone) {
    // reset flag
    operationDone = false;

    if(transmitFlag) {
      // the previous operation was transmission, listen for response
      // print the result
      if (transmissionState == RADIOLIB_ERR_NONE) {
        // packet was successfully sent
        Serial.println(F("transmission finished!"));        

      } else {
        Serial.print(F("failed, code "));
        Serial.println(transmissionState);

      }
      //radio.finishTransmit();
      //turn LED off
      digitalWrite(2, LOW);   // turn the LED off by making the voltage LOW        
      radio.setFrequency(RX_freq);
      
      // listen for response
      radio.startReceive();
      transmitFlag = false;

    } else {
      // the previous operation was reception
      // print data and send another packet
      String str;
      int state = radio.readData(str);


      if (state == RADIOLIB_ERR_NONE) {
        // packet was successfully received
        Serial.println(F("[SX1262] Received packet!"));

        // print data of the packet
        Serial.print(F("[SX1262] Data:\t\t"));
        Serial.println(str);

        // print RSSI (Received Signal Strength Indicator)
        Serial.print(F("[SX1262] RSSI:\t\t"));
        Serial.print(radio.getRSSI());
        Serial.println(F(" dBm"));

        // print SNR (Signal-to-Noise Ratio)
        Serial.print(F("[SX1262] SNR:\t\t"));
        Serial.print(radio.getSNR());
        Serial.println(F(" dB"));
        //send UDP
        udp.broadcast(str.c_str());
      }    
      else {
        Serial.print(F("failed, code "));
        Serial.println(state);    
      }     
    }
    // wait before transmitting again
      delay(500);     
  }  
}
