/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-webserial-library/
  
  This sketch is based on the WebSerial library example: ESP32_Demo
  https://github.com/ayushsharma82/WebSerial
*/

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define LED 2

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;

String bt_rec;
String bt_send;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// include the library
#include <RadioLib.h>

// SX1262 has the following connections:
// NSS pin:   10
// DIO1 pin:  2
// NRST pin:  3
// BUSY pin:  9
//SX1262 radio1 = new Module(10, 2, 3, 9);

float Freq_error = 0.001803;
float RX_freq = 262.225 + Freq_error; 
float TX_freq = 262.225+33.6 + Freq_error;


SX1262 radio = new Module(5, 21, 33, 26);

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

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {   
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
     
	  if (value.length() > 0) {
        bt_rec = "";
        for (int i = 0; i < value.length(); i++){
          // Serial.print(value[i]);
          bt_rec = bt_rec + value[i];
        }

        Serial.print("BLE Received = ");
        Serial.println(bt_rec);
         //send to lora
        transmit(bt_rec);
		
        /*if(bt_rec == "on") {
          //turn LED on
          digitalWrite(2, HIGH);   // turn the LED on by making the voltage HIGH
          //pCharacteristic->setValue(bt_send1);
          }
        if(bt_rec == "off") {
          //turn LED off
          digitalWrite(2, LOW);   // turn the LED off by making the voltage LOW 
          //pCharacteristic->setValue(bt_send2.c_str()); 
          }        
        */
      }
    }
};

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);

  BLEDevice::init("ESP32 Lora");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->setValue("Init.");
  pService->start();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();


  // initialize SX1262 with default settings
  Serial.print(F("[SX1262] Initializing ... "));

  //7.81 , 10.42 , 15.63, 20.83 
  //20.83
  //int state = radio.begin(262.225+33.6, 15.63, 9, 5, 0x34, 22, 8, 0); // works
  //int state = radio.begin(262.226+33.6, 20.83, 9, 5, 0x34, 22, 8, 0); // works
  int state = radio.begin(RX_freq, 20.83, 9, 5, 0x34, 22, 8, 0); // works
  //radio.setTCXO(3.3, 1563);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("Init failed, code "));
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
  if (deviceConnected) {    
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
          Serial.print(F("TX failed, code "));
          Serial.println(transmissionState);
          bt_send = "TX failed";

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
          //send to client
          bt_send = str;
        }    
        else {
          Serial.print(F("RX failed, code "));
          Serial.println(state);
          bt_send = "RX failed";
        }     
      }
      //send BLE
      pCharacteristic->setValue(bt_send.c_str()); 
      pCharacteristic->notify();
      delay(5); // bluetooth stack will go into congestion, if too many packets are sent.
      // wait before transmitting again
        delay(500);     
    }  
  }
  if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising     
        Serial.println("start listen BLE");
        oldDeviceConnected = deviceConnected;   
    }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
}
