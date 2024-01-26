#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include "DHT.h"
#include <addons/RTDBHelper.h>


byte MasterNode = 0xFF;
byte Node1 = 0xBB;

String SenderNode = "";
String outgoing;  // outgoing message

byte msgCount = 0;  // count of outgoing messages
String incoming = "";

int Sensor1 = 0;  // Potentiometer
int Sensor2 = 0;  // Flame Sensor
int Sensor3 = 0;  // LDR Sensor
float Sensor4 = 0.0;  // LDR Sensor
float Sensor5 = 0.0;  // LDR Sensor
int Sensor6 = 0;  // LDR Sensor

String DayNight = "Day";
String MotorStatus;


#define WIFI_SSID "ibrar"
#define WIFI_PASSWORD "ibrar12345"

/* 2. Define the RTDB URL */
#define DATABASE_URL "lab14task3dht11-default-rtdb.firebaseio.com" //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app

/* 3. Define the Firebase Data object */
FirebaseData fbdo;

/* 4, Define the FirebaseAuth data for authentication data */
FirebaseAuth auth;

/* Define the FirebaseConfig data for config data */
FirebaseConfig config;

unsigned long dataMillis = 0;
int count = 0;

/////////////////////////////////////////////////////////////////////////////////////////////
String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void onReceive(int packetSize) {
  if (packetSize == 0) return;  // if there's no packet, return
  // read packet header bytes:
  int recipient = LoRa.read();  // recipient address
  byte sender = LoRa.read();    // sender address
  if (sender == 0XBB)
    SenderNode = "Node1:";
  byte incomingMsgId = LoRa.read();   // incoming msg ID
  byte incomingLength = LoRa.read();  // incoming msg length

  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }

  if (incomingLength != incoming.length()) {  // check length for error
    //Serial.println("error: message length does not match length");
    return;  // skip rest of function
  }
  // if the recipient isn't this device or broadcast,
  if (recipient != Node1 && recipient != MasterNode) {
    // Serial.println("This message is not for me.");
    return;  // skip rest of function
  }

  // if message is for this device, or broadcast, print details:
  //Serial.println("Received from: 0x" + String(sender, HEX));
  //Serial.println("Sent to: 0x" + String(recipient, HEX));
  //Serial.println("Message ID: " + String(incomingMsgId));
  // Serial.println("Message length: " + String(incomingLength));
  // Serial.println("Message: " + incoming);
  //Serial.println("RSSI: " + String(LoRa.packetRssi()));
  // Serial.println("Snr: " + String(LoRa.packetSnr()));
  // Serial.println();

  String q = getValue(incoming, ',', 0);  // Pot
  String r = getValue(incoming, ',', 1);  // Flame Sensor
  String s = getValue(incoming, ',', 2);  // LDR
  String t = getValue(incoming, ',', 3);  // LDR
  String h = getValue(incoming, ',', 4);  // LDR
  String m = getValue(incoming, ',', 5);  // LDR

  Sensor1 = q.toInt();
  Sensor2 = r.toInt();
  Sensor3 = s.toInt();
  Sensor4 = t.toFloat();
  Sensor5 = h.toFloat();
  Sensor6 = m.toInt();

  if (Firebase.setInt(fbdo, "/Temperature", Sensor4)) {
    Serial.println("Temperature sent to Firebase successfully");
  } else {
    Serial.println("Error sending temperature to Firebase: " + fbdo.errorReason());
  }

  if (Firebase.setInt(fbdo, "/Humidity", Sensor5)) {
    Serial.println("Humidity sent to Firebase successfully");
  } else {
    Serial.println("Error sending humidity to Firebase: " + fbdo.errorReason());
  }

  if (Firebase.setInt(fbdo, "/MoistureSensor:", Sensor2)) {
    Serial.println("MoistureSensor sent to Firebase successfully");
  } else {
    Serial.println("Error sending MoistureSensor to Firebase: " + fbdo.errorReason());
  }

  if (Firebase.setInt(fbdo, "/RAINSENSOR", Sensor1)) {
    Serial.println("RAINSENSOR sent to Firebase successfully");
  } else {
    Serial.println("Error sending RAINSENSOR to Firebase: " + fbdo.errorReason());
  }

  if (Sensor3 == 1) {
    DayNight = "Night";
  }
  if (Sensor3 == 0) {
    DayNight = "Day";
  }
    if (Sensor6 == 1) {
    MotorStatus = "Motor ON";
  }
  if (Sensor3 == 0) {
    MotorStatus = "Motor OFF";
  }


  if (Firebase.setInt(fbdo, "/DayNight", Sensor3)) {
    Serial.println("DN sent to Firebase successfully");
  } else {
    Serial.println("Error sending DN to Firebase: " + fbdo.errorReason());
  }
  if (Firebase.setString(fbdo, "/Motor Status", MotorStatus)) {
    Serial.println("Motor Status sent to Firebase successfully");
  } else {
    Serial.println("Error sending Motor Status to Firebase: " + fbdo.errorReason());
  }
  incoming = "";
  Serial.print(SenderNode);
  Serial.println("RAINSENSOR:" + String(Sensor1));
  Serial.println("MoistureSensor:" + String(Sensor2));
  Serial.println("LDR:" + DayNight);
  Serial.println("temp:" + String(Sensor4));
  Serial.println("hum:" + String(Sensor5));
  Serial.println("MotorStatus:" + String(Sensor6));
}


//////////////////////////////////////////////////////////////////////
void setup()
{

    Serial.begin(115200);

    Serial.println("LoRa Receiver");
    LoRa.setPins(5, 4, 2);
    if (!LoRa.begin(433E6)) {
      Serial.println("Starting LoRa failed!");
      while (1);
  }
 

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    

    Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

    /* Assign the certificate file (optional) */
    // config.cert.file = "/cert.cer";
    // config.cert.file_storage = StorageType::FLASH;

    /* Assign the database URL(required) */
    config.database_url = DATABASE_URL;

    config.signer.test_mode = true;

    /**
     Set the database rules to allow public read and write.

       {
          "rules": {
              ".read": true,
              ".write": true
          }
        }

    */

    // Comment or pass false value when WiFi reconnection will control by your code or third party library e.g. WiFiManager
    Firebase.reconnectNetwork(true);

    // Since v4.4.x, BearSSL engine was used, the SSL buffer need to be set.
    // Large data transmission may require larger RX buffer, otherwise connection issue or data read time out can be occurred.
    fbdo.setBSSLBufferSize(4096 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);

    /* Initialize the library with the Firebase authen and config */
    Firebase.begin(&config, &auth);
    
}

void loop()
{
  onReceive(LoRa.parsePacket());

  

  



  delay(500);
}


