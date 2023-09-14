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
#include <BLE2902.h>

#define LED 2

#define SERVICE_UUID         "074a6272-6972-4e0a-bec4-fc21bc0638bd"
#define CHARACTERISTIC1_UUID "ebf8d607-c368-49a2-99e3-1e591dcc4472"
#define CHARACTERISTIC2_UUID "b6aa8f7e-a045-4b5e-be3d-3d15a7b74661"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
BLECharacteristic* pCharacteristic2 = NULL;
BLEDescriptor *pDescr;
BLE2902 *pBLE2902;

String bt_rec;
String bt_send;
String bt_send2;
bool deviceConnected = false;
bool oldDeviceConnected = false;

//uint16_t mtu = 128;

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

  // Create the BLE Device
  BLEDevice::init("ESP32 Lora");
   
  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
   // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC1_UUID,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );                   

  pCharacteristic2 = pService->createCharacteristic(
                      CHARACTERISTIC2_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  
                    );  

  // Create a BLE Descriptor
  pDescr = new BLEDescriptor((uint16_t)0x2901);
  pDescr->setValue("Data from Lora");
  pCharacteristic->addDescriptor(pDescr);
  
  pBLE2902 = new BLE2902();
  pBLE2902->setNotifications(true);
  
  // Add all Descriptors here
  pCharacteristic->addDescriptor(pBLE2902);
  pCharacteristic2->addDescriptor(new BLE2902());
  
  // After defining the desriptors, set the callback functions
  pCharacteristic2->setCallbacks(new MyCallbacks());
  //pCharacteristic2->setCallbacks(new CharacteristicCallBack());

  //pCharacteristic->setValue("Init.");
  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();

  
  //BLEDevice::setMTU(mtu);

  Serial.println("Waiting a client connection to notify...");

//  BLEAdvertising *pAdvertising = pServer->getAdvertising();
//  pAdvertising->start();


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
          bt_send = "TX finished";      

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
          bt_send = "New message";
          bt_send2 = str;
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
      delay(5);
      pCharacteristic2->setValue(bt_send2.c_str()); 
      pCharacteristic2->notify();
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
