#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>

#define bleServerName "Cosmin's and Virigil's ESP32"
bool connected = false;
#define SERVICE_UUID "cdd6731b-f583-423b-9b16-07bcea65fe61"
#define CHARACTERISTIC_UUID "5c838202-5944-48e4-9b02-56a351533deb"
char TEAM_ID[20] = "team-id";
DynamicJsonDocument doc(4096);

const char* ssid = ""; 
const char* password = ""; 

BLECharacteristic characteristic(
  CHARACTERISTIC_UUID,
  BLECharacteristic :: PROPERTY_READ |
  BLECharacteristic :: PROPERTY_WRITE | 
  BLECharacteristic :: PROPERTY_NOTIFY
);

BLEDescriptor *characteristicDescriptor = new BLEDescriptor(BLEUUID((uint16_t) 0x2902));

class MyServerCallbacks : public BLEServerCallbacks{
  void onConnect(BLEServer* pServer){
    connected = true;
    Serial.println("Device connected");
  }

  void onDisconnect(BLEServer* pServer){
    connected = false;
    Serial.println("Device disconnected");
  }
};

class CharacteristicCallbacks : public BLECharacteristicCallbacks{
  void onWrite(BLECharacteristic* characteristic){
      std :: string data = characteristic -> getValue();
      Serial.println(data.c_str());
      deserializeJson(doc, data.c_str());
      std::string action = doc["action"]; 
      std::string teamId = doc["teamId"]; 
      std::string ssid = doc["ssid"]; 
      std::string password = doc["password"]; 

      if(action.compare("getNetworks") == 0){
          // Serial.println("Available networks are: ");
  
          // WiFi.disconnect();
          // WiFi.mode(WIFI_STA);

          int nr_networks = WiFi.scanNetworks();

          if(nr_networks == 0){
            Serial.println("No networks found");
          }
          else{
            for(int i = 0; i <= nr_networks - 1; ++i){
              //Serial.println(WiFi.SSID(i));
              DynamicJsonDocument jsonDoc(4096);
              JsonObject network = jsonDoc.to<JsonObject>();
              network["ssid"] = WiFi.SSID(i);
              network["strength"] = WiFi.RSSI(i);
              network["encryption"] = WiFi.encryptionType(i);
              network["teamId"] = teamId;
              strcpy(TEAM_ID, teamId.c_str());
              std::string jsonData;
              serializeJson(jsonDoc,jsonData);
              serializeJson(jsonDoc,Serial);
              characteristic->setValue(jsonData);
              characteristic->notify();
              //Serial.println("");
              delay(10);
            }
          }
      }
      else if(action.compare("connect") == 0){
          Serial.println(action.c_str());
          Serial.println(ssid.c_str());
          Serial.println(password.c_str());
          WiFi.begin(ssid.c_str(), password.c_str());
          delay(1000);

          //Check status
          if(WiFi.status() != WL_CONNECTED){
            Serial.println("Not connected");
          }
          else{
            Serial.println(WiFi.localIP());

            DynamicJsonDocument jsonDoc(4096);
            JsonObject network = jsonDoc.to<JsonObject>();
            network["ssid"] = WiFi.SSID();
            network["connected"] = true;
            network["teamId"] = TEAM_ID;
            std::string jsonData;
            serializeJson(jsonDoc,jsonData);
            serializeJson(jsonDoc,Serial);
            characteristic->setValue(jsonData);
            characteristic->notify();
            delay(10);
          }
      }
      else if(action.compare("getData") == 0){
        String url = "http://proiectia.bogdanflorea.ro/api/futurama/characters";

        HTTPClient http;
        http.begin(url);
        int httpCode = http.GET();

        if(httpCode == HTTP_CODE_OK){
          String Payload = http.getString();
          DynamicJsonDocument jsonDoc(4096);
          deserializeJson(doc, Payload);

          JsonArray jsonArr = doc.as<JsonArray>();

          for(JsonObject obj : jsonArr){
            if(obj.containsKey("PicUrl")){  
              obj["image"] = obj["PicUrl"];
              obj.remove("PicUrl");
            }

            if(obj.containsKey("Id")){  
              obj["id"] = obj["Id"];
              obj.remove("Id");
            }
            
            obj["teamId"] = TEAM_ID;
            DynamicJsonDocument jsonDoc(4096);
            jsonDoc.set(obj);
            std :: string jsonData;
            serializeJson(jsonDoc, jsonData);
            characteristic->setValue(jsonData);
            characteristic->notify();
            delay(10);
          }
        }
      }
      else if(action.compare("getDetails") == 0){
        std :: string Id = doc["id"];
        Serial.println(Id.c_str());
        String url = "http://proiectia.bogdanflorea.ro/api/futurama/character?Id=";
        String url_final = String(url.c_str()) + String(Id.c_str());
        Serial.println(url_final.c_str());
        HTTPClient http;
        http.begin(url_final);

        int httpCode = http.GET();

        if(httpCode == HTTP_CODE_OK){
          String Payload = http.getString();
          DynamicJsonDocument doc(4096);
          deserializeJson(doc, Payload);

          int IdCaracter = doc["Id"];
          String name = doc["Name"];
          String image = doc["PicUrl"];
          String species = doc["Species"];
          String age = doc["Age"];
          String planet = doc["Planet"];
          String proffesion = doc["Profession"];
          String status = doc["Status"];
          String firstap = doc["FirstAppearance"];
          String relatives = doc["Relatives"];
          String voicedby = doc["VoicedBy"];
          String description = String("Species : ") + species + String("\n") + 
                              String("Age : ") + age + String("\n") + 
                              String("Planet : ") + planet + String("\n") +
                              String("Profession : ") + proffesion + String("\n") + 
                              String("Status : ") + status + String("\n") + 
                              String("First Appearance : ") + firstap + String("\n") + 
                              String("Relatives : ") + relatives + String("\n") + 
                              String("Voiced by : ") + voicedby;

          DynamicJsonDocument jsonDoc(4096);
          JsonObject character = jsonDoc.to<JsonObject>();
          character["Id"] = IdCaracter;
          character["name"] = name;
          character["image"] = image;
          character["description"] = description;
          character["teamId"] = TEAM_ID;

          std::string jsonData;
          serializeJson(jsonDoc, jsonData);
          characteristic->setValue(jsonData);
          characteristic->notify();
          delay(10);
        }
      }

  }
};


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  BLEDevice :: init (bleServerName); //BLE device
  BLEServer *pServer = BLEDevice :: createServer(); //BLE server
  pServer -> setCallbacks(new MyServerCallbacks());
  BLEService *BLEService = pServer -> createService(SERVICE_UUID); //BLE service
  BLEService -> addCharacteristic(&characteristic); //BLE characteristic
  characteristic.addDescriptor(characteristicDescriptor); //BLE descriptor
  characteristic.setCallbacks(new CharacteristicCallbacks());
  BLEService -> start();
  BLEAdvertising *pAdvertising = BLEDevice :: getAdvertising();
  pAdvertising -> addServiceUUID(SERVICE_UUID);
  pServer -> getAdvertising() -> start();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
  // put your main code here, to run repeatedly:
}