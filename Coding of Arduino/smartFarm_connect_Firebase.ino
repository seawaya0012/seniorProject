#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <SimpleDHT.h>
#include <BH1750FVI.h>
#include "FirebaseESP8266.h"
#include <Servo.h>

Servo myservo; 

// FirebaseESP8266.h must be included before ESP8266WiFi.h
#define FIREBASE_HOST "senior-project-3a959-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "YJthemhebjvy5Ivkp4EJmqGcFc70t1FIq5yHDhRP"
#define WIFI_SSID "123123123"
#define WIFI_PASSWORD "123123123"

WiFiServer server(80);

// Define FirebaseESP8266 data object
FirebaseData firebaseData;
FirebaseJson json;
void printResult(FirebaseData &data);

// Root path
String path1 = "/ESP_Device";
String path2 = "/ESP_Controll";
String path3 = "/ESP_Status";

// Lux Config
BH1750FVI LightSensor(BH1750FVI::k_DevModeContLowRes);

// WiFi credentials.
char ssid[] = "123123123";
char pass[] = "123123123";

// Config Relay
int LED = D7;
int valvepin = D6;

void setup()
{
  Serial.begin(115200);
  LightSensor.begin();
  pinMode(LED, OUTPUT); // กำหนดขาทำหน้าที่ให้ขา D3 เป็น OUTPUT 
  myservo.attach(D4);

  pinMode(valvepin, OUTPUT); //Relay valv
  
  initWifi();

  Serial.println("Connected");
  Serial.println("Your IP is");
  Serial.println((WiFi.localIP().toString()));
  server.begin();

}

void initWifi(){
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

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  firebaseData.setBSSLBufferSize(1024, 1024);
  firebaseData.setResponseSize(1024);
  Firebase.setReadTimeout(firebaseData, 1000 * 60);
  Firebase.setwriteSizeLimit(firebaseData, "tiny");
}



String sendTemp(String statusCooling, String command)
{
  int pinDHT22 = D5;
  SimpleDHT22 dht22;
  float temperature = 0;
  float humidity = 0;
  int err = SimpleDHTErrSuccess;
  if ((err = dht22.read2(pinDHT22, &temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
    Serial.print("Read DHT22 failed, err="); Serial.println(err);delay(2000);
  }  
  Serial.print("Temperature: "); Serial.print(temperature); Serial.print(" Humidity: "); Serial.println(humidity);
  String stringTemperature = "Temperature is " + String(temperature);
  String stringHumidity = "Humidity is " + String(humidity);
  Firebase.setString(firebaseData, path1 + "/ATemperature/StatusofFarm", stringTemperature);
  Firebase.setString(firebaseData, path1 + "/BHumidity/StatusofFarm", stringHumidity);
  int temperatures = int(temperature);
  int humiditys = int(humidity);
  int num=0;
  if(command == "Auto"){
    if(temperatures <= 30 && humiditys <= 90 && num == 0){
      Serial.println("\nCooling OFF");
      statusCooling = "Cooling : OFF";
      myservo.write(75); // สั่งให้ Servo หมุนไปองศาที่ 0
      delay(100); // หน่วงเวลา 1000ms
      myservo.write(105);
      num = 1;
      Firebase.setString(firebaseData, path2 + "/Detect_RelayCooling/controll", statusCooling);
    }else if (temperatures > 30 && humiditys > 75 && num == 1){
      Serial.println("\nCooling ON");
      statusCooling = "Cooling : ON";
      myservo.write(75); // สั่งให้ Servo หมุนไปองศาที่ 0
      delay(100); // หน่วงเวลา 1000ms
      myservo.write(105);
      num = 0;
      Firebase.setString(firebaseData, path2 + "/Detect_RelayCooling/controll", statusCooling);
    } 
  }
  return statusCooling;
}


String sendWater(String statusSolenoid, String command){
  int sensorValue = analogRead(A0);
  Serial.print("sensorValue : ");
  Serial.println(sensorValue);
  String stringSensorValue = "Water is " + String(sensorValue);
  Firebase.setString(firebaseData, path1 + "/Detect_Water/StatusofFarm", stringSensorValue);
  if(command == "Auto"){
    if(sensorValue = 0){
      Serial.println("\nSolenoid ON");
      statusSolenoid = "Solenoid : ON";
      digitalWrite(valvepin, HIGH);
      Firebase.setString(firebaseData, path2 + "/Detect_RelaySolenoid/controll", statusSolenoid);
    }else{
      Serial.println("\nSolenoid OFF");
      statusSolenoid = "Solenoid : OFF";
      digitalWrite(valvepin, LOW);
      Firebase.setString(firebaseData, path2 + "/Detect_RelaySolenoid/controll", statusSolenoid);
    }
  }
  return statusSolenoid;
}

String sendlux(String statusLED, String command){
  uint16_t lux;
  lux = LightSensor.GetLightIntensity();
  Serial.print("Light: ");
  Serial.print(lux);
  Serial.println(" lux");
  String stringLux = "Lux is " + String(lux);
  Firebase.setString(firebaseData, path1 + "/CLux/StatusofFarm", stringLux);
  if(command == "Auto"){
    if(lux > 15000){
      Serial.println("\nLED OFF");
      statusLED = "LED : OFF";
      digitalWrite(LED, LOW);
      Firebase.setString(firebaseData, path2 + "/Detect_RelayLED/controll", statusLED);
    }else if(lux < 10000){
      Serial.println("\nLED ON");
      statusLED = "LED : ON";
      digitalWrite(LED, HIGH);
      Firebase.setString(firebaseData, path2 + "/Detect_RelayLED/controll", statusLED);
    }else {
      Serial.println("\nLED ON");
      statusLED = "LED : ON";
      digitalWrite(LED, LOW);
      Firebase.setString(firebaseData, path2 + "/Detect_RelayLED/controll", statusLED);
    }
  }
  return statusLED;
}

void relay(){
  String command = "";
  String autos = "";
  
  String statusLED = "";
  String statusCooling = "";
  String statusSolenoid = "";
  statusCooling = sendTemp(statusCooling, "Auto");
  statusLED = sendlux(statusLED, "Auto");
  statusSolenoid = sendWater(statusSolenoid, "Auto");
  WiFiClient client = server.available();
  if(client){
    while (client.connected()){
      if(client.available()>0){
        if(command == "Auto"){
          command = "";
        }
      }
      while (client.available()>0) {
        char c = client.read();
        if (c == '\n') {
          break;
        }
        command += c;
        Serial.write(c);
        Serial.print("\n");
        Serial.println(command);
      }
      if(command == "Auto"){
        autos = "Auto : ON";
        Firebase.setString(firebaseData, path3 + "/Status_Mode/Mode", autos);
      }else if(command != "Auto"){
        autos = "Auto : OFF";
        Firebase.setString(firebaseData, path3 + "/Status_Mode/Mode", autos);
      }
      
      if(command == "LED" && statusLED == "LED : OFF"){
        Serial.println("\nON");
        statusLED = "LED : ON";
        digitalWrite(LED, HIGH);
        Firebase.setString(firebaseData, path2 + "/Detect_RelayLED/controll", statusLED);
      }else if(command == "LED" && statusLED == "LED : ON"){
        Serial.println("\nOFF");
        statusLED = "LED : OFF";
        digitalWrite(LED, LOW);
        Firebase.setString(firebaseData, path2 + "/Detect_RelayLED/controll", statusLED);
      }
      
      if(command == "Cooling" && statusCooling == "Cooling : OFF"){
        Serial.println("\nON");
        statusCooling = "Cooling : ON";
        myservo.write(75); // สั่งให้ Servo หมุนไปองศาที่ 0
        delay(100); // หน่วงเวลา 1000ms
        myservo.write(105);
        Firebase.setString(firebaseData, path2 + "/Detect_RelayCooling/controll", statusCooling);
      }else if(command == "Cooling" && statusCooling == "Cooling : ON"){
        Serial.println("\nOFF");
        statusCooling = "Cooling : OFF";
        myservo.write(75); // สั่งให้ Servo หมุนไปองศาที่ 0
        delay(100); // หน่วงเวลา 1000ms
        myservo.write(105);
        Firebase.setString(firebaseData, path2 + "/Detect_RelayCooling/controll", statusCooling);
      }
      
      if(command == "Solenoid" && statusSolenoid == "Solenoid : OFF"){
        Serial.println("\nON");
        statusSolenoid = "Solenoid : ON";
        digitalWrite(valvepin, HIGH);
        Firebase.setString(firebaseData, path2 + "/Detect_RelaySolenoid/controll", statusSolenoid);
      }else if(command == "Solenoid" && statusSolenoid == "Solenoid : ON"){
        Serial.println("\nOFF");
        statusSolenoid = "Solenoid : OFF";
        digitalWrite(valvepin, LOW);
        Firebase.setString(firebaseData, path2 + "/Detect_RelaySolenoid/controll", statusSolenoid);
      }
      if(command != "Auto"){
        command = "";
      }
      sendTemp(statusCooling, command);
            if(client.available()>0){
        if(command == "Auto"){
          command = "";
        }
      }
      while (client.available()>0) {
        char c = client.read();
        if (c == '\n') {
          break;
        }
        command += c;
        Serial.write(c);
        Serial.print("\n");
        Serial.println(command);
      }
      if(command == "Auto"){
        autos = "Auto : ON";
        Firebase.setString(firebaseData, path3 + "/Status_Mode/Mode", autos);
      }else if(command != "Auto"){
        autos = "Auto : OFF";
        Firebase.setString(firebaseData, path3 + "/Status_Mode/Mode", autos);
      }
      
      if(command == "LED" && statusLED == "LED : OFF"){
        Serial.println("\nON");
        statusLED = "LED : ON";
        digitalWrite(LED, HIGH);
        Firebase.setString(firebaseData, path2 + "/Detect_RelayLED/controll", statusLED);
      }else if(command == "LED" && statusLED == "LED : ON"){
        Serial.println("\nOFF");
        statusLED = "LED : OFF";
        digitalWrite(LED, LOW);
        Firebase.setString(firebaseData, path2 + "/Detect_RelayLED/controll", statusLED);
      }
      
      if(command == "Cooling" && statusCooling == "Cooling : OFF"){
        Serial.println("\nON");
        statusCooling = "Cooling : ON";
        myservo.write(75); // สั่งให้ Servo หมุนไปองศาที่ 0
        delay(100); // หน่วงเวลา 1000ms
        myservo.write(105);
        Firebase.setString(firebaseData, path2 + "/Detect_RelayCooling/controll", statusCooling);
      }else if(command == "Cooling" && statusCooling == "Cooling : ON"){
        Serial.println("\nOFF");
        statusCooling = "Cooling : OFF";
        myservo.write(75); // สั่งให้ Servo หมุนไปองศาที่ 0
        delay(100); // หน่วงเวลา 1000ms
        myservo.write(105);
        Firebase.setString(firebaseData, path2 + "/Detect_RelayCooling/controll", statusCooling);
      }
      
      if(command == "Solenoid" && statusSolenoid == "Solenoid : OFF"){
        Serial.println("\nON");
        statusSolenoid = "Solenoid : ON";
        digitalWrite(valvepin, HIGH);
        Firebase.setString(firebaseData, path2 + "/Detect_RelaySolenoid/controll", statusSolenoid);
      }else if(command == "Solenoid" && statusSolenoid == "Solenoid : ON"){
        Serial.println("\nOFF");
        statusSolenoid = "Solenoid : OFF";
        digitalWrite(valvepin, LOW);
        Firebase.setString(firebaseData, path2 + "/Detect_RelaySolenoid/controll", statusSolenoid);
      }
      if(command != "Auto"){
        command = "";
      }
      
      sendlux(statusLED, command);
            if(client.available()>0){
        if(command == "Auto"){
          command = "";
        }
      }
      while (client.available()>0) {
        char c = client.read();
        if (c == '\n') {
          break;
        }
        command += c;
        Serial.write(c);
        Serial.print("\n");
        Serial.println(command);
      }
      if(command == "Auto"){
        autos = "Auto : ON";
        Firebase.setString(firebaseData, path3 + "/Status_Mode/Mode", autos);
      }else if(command != "Auto"){
        autos = "Auto : OFF";
        Firebase.setString(firebaseData, path3 + "/Status_Mode/Mode", autos);
      }
      
      if(command == "LED" && statusLED == "LED : OFF"){
        Serial.println("\nON");
        statusLED = "LED : ON";
        digitalWrite(LED, HIGH);
        Firebase.setString(firebaseData, path2 + "/Detect_RelayLED/controll", statusLED);
      }else if(command == "LED" && statusLED == "LED : ON"){
        Serial.println("\nOFF");
        statusLED = "LED : OFF";
        digitalWrite(LED, LOW);
        Firebase.setString(firebaseData, path2 + "/Detect_RelayLED/controll", statusLED);
      }
      
      if(command == "Cooling" && statusCooling == "Cooling : OFF"){
        Serial.println("\nON");
        statusCooling = "Cooling : ON";
        myservo.write(75); // สั่งให้ Servo หมุนไปองศาที่ 0
        delay(100); // หน่วงเวลา 1000ms
        myservo.write(105);
        Firebase.setString(firebaseData, path2 + "/Detect_RelayCooling/controll", statusCooling);
      }else if(command == "Cooling" && statusCooling == "Cooling : ON"){
        Serial.println("\nOFF");
        statusCooling = "Cooling : OFF";
        myservo.write(75); // สั่งให้ Servo หมุนไปองศาที่ 0
        delay(100); // หน่วงเวลา 1000ms
        myservo.write(105);
        Firebase.setString(firebaseData, path2 + "/Detect_RelayCooling/controll", statusCooling);
      }
      
      if(command == "Solenoid" && statusSolenoid == "Solenoid : OFF"){
        Serial.println("\nON");
        statusSolenoid = "Solenoid : ON";
        digitalWrite(valvepin, HIGH);
        Firebase.setString(firebaseData, path2 + "/Detect_RelaySolenoid/controll", statusSolenoid);
      }else if(command == "Solenoid" && statusSolenoid == "Solenoid : ON"){
        Serial.println("\nOFF");
        statusSolenoid = "Solenoid : OFF";
        digitalWrite(valvepin, LOW);
        Firebase.setString(firebaseData, path2 + "/Detect_RelaySolenoid/controll", statusSolenoid);
      }
      if(command != "Auto"){
        command = "";
      }
      
      sendWater(statusSolenoid, command);
    }
    client.stop();
    Serial.println("Client disconnected");
  }
}

void loop(){
  relay();
}
