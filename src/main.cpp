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
#include <config.h>

#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <json.h>

#define LED 2

#define SERVICE_UUID "074a6272-6972-4e0a-bec4-fc21bc0638bd"
#define CHARACTERISTIC1_UUID "ebf8d607-c368-49a2-99e3-1e591dcc4472"
#define CHARACTERISTIC2_UUID "b6aa8f7e-a045-4b5e-be3d-3d15a7b74661"

JsonDocument ChannelJson;

float Freq_error = -0.000336;
float RX_freq = 300.000;
float Offset = 0;

float BW = 10.42;
float SYNC = 0x34;
int SF = 6;
int PWR = 22;
// float TCXO = 0;
float TCXO = 1.6;

// time of Ping
long int spankTime;

// BLE Config
BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
BLECharacteristic *pCharacteristic2 = NULL;
BLEDescriptor *pDescr;
BLE2902 *pBLE2902;

String bt_rec;
String bt_send;
String bt_send2;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// include the library
#include <RadioLib.h>

SX1262 radio = new Module(NSS, DIO1, NRST, BUSY);

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
void setFlag(void)
{
  // we sent or received a packet, set the flag
  operationDone = true;
}

void transmit(String &data)
{
  // turn LED on
  digitalWrite(2, HIGH); // turn the LED on (HIGH is the voltage level)

  Serial.println(F("[SX1262] Transmitting packet ... "));
  // int state = radio.startTransmit(data);
  radio.setFrequency(RX_freq + Offset + Freq_error);
  transmissionState = radio.startTransmit(data);
  transmitFlag = true;
}
void sendBLE(String bt_send, String bt_send2)
{
  // send BLE
  pCharacteristic->setValue(bt_send.c_str());
  pCharacteristic->notify();
  delay(5);
  pCharacteristic2->setValue(bt_send2.c_str());
  pCharacteristic2->notify();
  delay(5); // bluetooth stack will go into congestion, if too many packets are sent.
  // wait before transmitting again
  delay(500);
}

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    deviceConnected = true;
  };

  void onDisconnect(BLEServer *pServer)
  {
    deviceConnected = false;
  }
};

class MyCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string value = pCharacteristic->getValue();

    if (value.length() > 0)
    {
      bt_rec = "";
      for (int i = 0; i < value.length(); i++)
      {
        // Serial.print(value[i]);
        bt_rec = bt_rec + value[i];
      }

      Serial.print("BLE Received = ");
      Serial.println(bt_rec);

      if (bt_rec.substring(0, 5) == "@help")
      {
        String welcome = "Use the following commands to modem:\n";
        welcome += "@bw xx set bandwidth \n";
        welcome += "@rx xx set RX freq \n";
        welcome += "@of xx set offset \n";
        welcome += "@fe xx set freq error \n";
        welcome += "@sf xx set SF \n";
        welcome += "@pw xx set output power \n";
        welcome += "@st display current config \n";
        welcome += "@ch xx set channel \n";
        welcome += "@ping measure time \n";
        // sendBLE("New message", "test");
        sendBLE("New message", welcome);
        Serial.println(welcome);
      }
      else if (bt_rec.substring(0, 3) == "@bw")
      {
        Serial.print("Set bandwidth to ");
        Serial.println(bt_rec.substring(3, sizeof(bt_rec)).toFloat());
        BW = bt_rec.substring(3, sizeof(bt_rec)).toFloat();
        radio.setBandwidth(bt_rec.substring(3, sizeof(bt_rec)).toFloat());
        sendBLE("New message", "Bandwidth setted");
      }
      else if (bt_rec.substring(0, 3) == "@rx")
      {
        Serial.print("Set RX freq to ");
        Serial.println(RX_freq);
        RX_freq = bt_rec.substring(3, sizeof(bt_rec)).toFloat();
        sendBLE("New message", "RX freq setted");
      }
      else if (bt_rec.substring(0, 3) == "@of")
      {
        Serial.print("Set Offset freq to ");
        Serial.println(Offset);
        Offset = bt_rec.substring(3, sizeof(bt_rec)).toFloat();
        sendBLE("New message", "Offset setted");
      }
      else if (bt_rec.substring(0, 3) == "@fe")
      {
        Serial.print("Set freq error to ");
        Serial.println(Freq_error);
        Freq_error = bt_rec.substring(3, sizeof(bt_rec)).toFloat();
        sendBLE("New message", "Freq error setted");
      }
      else if (bt_rec.substring(0, 3) == "@sf")
      {
        Serial.print("Set SF to ");
        Serial.println(bt_rec.substring(3, sizeof(bt_rec)).toInt());
        SF = bt_rec.substring(3, sizeof(bt_rec)).toInt();
        radio.setSpreadingFactor(bt_rec.substring(3, sizeof(bt_rec)).toInt());
        sendBLE("New message", "SF setted");
      }
      else if (bt_rec.substring(0, 3) == "@pw")
      {
        Serial.print("Set PW to ");
        Serial.println(bt_rec.substring(3, sizeof(bt_rec)).toInt());
        PWR = bt_rec.substring(3, sizeof(bt_rec)).toFloat();
        radio.setSpreadingFactor(bt_rec.substring(3, sizeof(bt_rec)).toInt());
        sendBLE("New message", "PWR setted");
      }
      else if (bt_rec.substring(0, 3) == "@st")
      {
        String conf = "Config:\n";
        conf += "@rx = " + String(RX_freq, 3) + "\n";
        conf += "@of = " + String(Offset, 3) + "\n";
        conf += "@bw = " + String(BW) + "\n";
        conf += "@sf = " + String(SF) + "\n";
        conf += "@fe = " + String(Freq_error, 6) + "\n";
        conf += "@pw = " + String(PWR) + "\n";
        sendBLE("New message", conf);
      }
      else if (bt_rec.substring(0, 3) == "@ch")
      {
        int channel = bt_rec.substring(3, sizeof(bt_rec)).toInt();
        if (channel > ChannelJson.size())
        {
          channel = ChannelJson.size();
        }
        SetChannel(channel, ChannelJson, radio);

        String conf = PrintChannel(channel, ChannelJson);
        sendBLE("New message", conf);
      }
      else if (bt_rec.substring(0, 5) == "@ping")
      {
        bt_rec = "";
        radio.setSpreadingFactor(6);
        transmit(bt_rec);
        spankTime = millis();
      }
      else if (bt_rec.substring(0, 5) == "@list")
     {
       String conf = ListChannel(ChannelJson);
       sendBLE("New message", conf);
     }
      else
      {
        transmit(bt_rec);
      }
    }

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
};

void setup()
{
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
      BLECharacteristic::PROPERTY_NOTIFY);

  pCharacteristic2 = pService->createCharacteristic(
      CHARACTERISTIC2_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE);

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
  // pCharacteristic2->setCallbacks(new CharacteristicCallBack());

  // pCharacteristic->setValue("Init.");
  //  Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0); // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();

  // BLEDevice::setMTU(mtu);

  Serial.println("Waiting a client connection to notify...");

  //  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  //  pAdvertising->start();

  // initialize SX1262 with default settings
  Serial.print(F("[SX1262] Initializing ... "));

  // int state = radio.begin(RX_freq + Freq_error, BW, SF, 5, SYNC, 22, 8, 0); // works
  int state = radio.begin(RX_freq + Freq_error, BW, SF, 5, SYNC, 22, 8, TCXO); // works
  // radio.setTCXO(3.3, 1563);

  if (state == RADIOLIB_ERR_NONE)
  {
    Serial.println(F("success!"));
  }
  else
  {
    Serial.print(F("Init failed, code "));
    Serial.println(state);
    while (true)
      ;
  }

  radio.setPacketSentAction(setFlag);
  radio.setPacketReceivedAction(setFlag);
  // radio.setDio1Action(setFlag);

  Serial.print(F("[SX1262] Starting to listen ... "));
  state = radio.startReceive();
  if (state == RADIOLIB_ERR_NONE)
  {
    Serial.println(F("success!"));
  }
  else
  {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true)
      ;
  }

  // read SPIFFS
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  File file = SPIFFS.open("/freq.json", "r");
  if (!file)
  {
    Serial.println("Failed to open file with freq for reading");
    return;
  }

  if (file && file.size())
  {

    DeserializationError err = deserializeJson(ChannelJson, file);
    Serial.println(err.c_str());
    if (err)
    {
      Serial.print(F("deserializeJson() failed with code "));
      Serial.println(err.c_str());
    }
    file.close();
  }
  else
  {
    Serial.println("Failed to read file.");
  }

  // Set default channel
  SetChannel(CHANNEL, ChannelJson, radio);
}

void loop()
{
  // reset ping time
  if (spankTime != 0 && millis() - spankTime > 1000)
  {
    spankTime = 0;
    radio.setSpreadingFactor(SF);
  }

  if (deviceConnected)
  {
    // check if the previous operation finished
    if (operationDone)
    {
      // reset flag
      operationDone = false;

      if (transmitFlag)
      {
        // the previous operation was transmission, listen for response
        // print the result
        if (transmissionState == RADIOLIB_ERR_NONE)
        {
          // packet was successfully sent
          Serial.println(F("transmission finished!"));
          bt_send = "TX finished";
        }
        else
        {
          Serial.print(F("TX failed, code "));
          Serial.println(transmissionState);
          bt_send = "TX failed";
        }
        // radio.finishTransmit();
        // turn LED off (need check)
        digitalWrite(2, LOW); // turn the LED off by making the voltage LOW
        radio.setFrequency(RX_freq + Freq_error);

        // listen for response
        radio.startReceive();
        transmitFlag = false;
      }
      else
      {
        // the previous operation was reception
        // print data and send another packet
        String str;
        int state = radio.readData(str);

        if (state == RADIOLIB_ERR_NONE)
        {
          if (str == "")
          {
            str = String((int)(millis() - spankTime));
            // send to client
            bt_send = "New message";
            bt_send2 = "delay=" + str + "ms" + " RX [RSSI: " + String(radio.getRSSI()) + " SNR: " + String(radio.getSNR()) + " ERROR: " + String(radio.getFrequencyError()) + " ]\n";
          }
          else
          {
            // packet was successfully received
            Serial.println(F("[SX1262] Received packet!"));

            String buf = "RX [RSSI: " + String(radio.getRSSI()) + " SNR: " + String(radio.getSNR()) + " ERROR: " + String(radio.getFrequencyError()) + " ]\n";

            // print data of the packet
            Serial.print(F("[SX1262] Data:\t\t"));
            Serial.println(str);

            // send to client
            bt_send = "New message";
            bt_send2 = buf + str;
          }
        }
        else
        {
          Serial.print(F("RX failed, code "));
          Serial.println(state);
          bt_send = "RX failed" + str;
        }
      }
      // send BLE
      sendBLE(bt_send, bt_send2);
    }
  }
  if (!deviceConnected && oldDeviceConnected)
  {
    delay(500);                  // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
    Serial.println("start listen BLE");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected)
  {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}
