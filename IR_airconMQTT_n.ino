/* IRremoteESP8266 over MQTT */
/* Aircon Smart Remocon for National A75C219. 2023 by diysmartmatter*/

#include <Arduino.h>
#include <ArduinoOTA.h>

//JSON
#include <ArduinoJson.h>
StaticJsonDocument<200> doc;

//IR Remote
#include "ir_National.h"
const uint16_t kIrLed = 4;  // ESP8266 GPIO pin to use. Recommended: 4 (D2).
IRNational national=IRNational(kIrLed);
uint8_t _CoolingThresholdTemperature = 26; //HK has specific temp-value for each mode 
uint8_t _HeatingThresholdTemperature = 22; //HK has specific temp-value for each mode

DHT sensor
#include "DHT20.h"
DHT20 dht; //instance of DHT20

//MQTT
#include <EspMQTTClient.h>
EspMQTTClient *client; //instance of MQTT client

//WiFi & MQTT
const char SSID[] = "xxxxxxxx"; //WiFi SSID

const char PASS[] = "XXXXXXXX"; //WiFi password
char CLIENTID[] = "ESP32_xx:xx:xx:xx:xx:xx"; //MAC address is set in setup()
const char  MQTTADD[] = "192.168.xxx.xxx"; //Broker IP address
const short MQTTPORT = 1883; //Broker port
const char  MQTTUSER[] = "";//Can be omitted if not needed
const char  MQTTPASS[] = "";//Can be omitted if not needed
const char  SUBTOPICH[] = "mqttthing/irNational/set/#"; //to subscrive HomeKit
const char  SUBTOPICF[] = "zigbee2mqtt/NationalFlap"; //to subscrive National AC Flap sensor
const char  PUBTOPICF[] = "mqttthing/irNational/get/Active"; //to publish flap state
const char  PUBTOPICT[] = "mqttthing/irNational/get"; //to publish temperature
const char  DEBUG[] = "mqttthing/irNational/debug"; //topic for debug

void setup() {
  Wire.begin();
  dht.begin(); //DHT20
  national.begin();
  Serial.begin(115200);
  while (!Serial);      //  wait for serial port to connect.
  String wifiMACString = WiFi.macAddress(); //WiFi MAC address
  wifiMACString.toCharArray(&CLIENTID[6], 18, 0); //"ESP32_xx:xx:xx:xx:xx:xx"
  client = new EspMQTTClient(SSID,PASS,MQTTADD,MQTTUSER,MQTTPASS,CLIENTID,MQTTPORT); 
  delay(1000);
}


//message has received from HomeKit
void onMessageReceivedH(const String& topic, const String& message) { 
  String command = topic.substring(topic.lastIndexOf("/") + 1);

  if (command.equals("Active")) {
    bool newState=false; //default for JIC
    if(message.equalsIgnoreCase("true")) newState=true;
    if(message.equalsIgnoreCase("false")) newState=false;
    if(national.updateState(newState)) { //if On/Off state is updated
      national.toggleOnOff();
    }
    client->publish(DEBUG,national.toChars());
    //national.send() is not required, because toggleOnOff() sends IR signal
  }else if(command.equals("TargetHeaterCoolerState")){
    if(message.equalsIgnoreCase("COOL")) {
      national.setTemp(_CoolingThresholdTemperature);//each mode has specific temp
      national.setMode(kNationalCool);
    }
    if(message.equalsIgnoreCase("HEAT")) {
      national.setTemp(_HeatingThresholdTemperature);//each mode has specific temp
      national.setMode(kNationalHeat);
    }
    //national.send() is not required, because HomeKit sends "Active" msg
  }else if(command.equals("CoolingThresholdTemperature")){
    national.setTemp(message.toInt());
    _CoolingThresholdTemperature=national.getTemp(); //may be capped
    client->publish(DEBUG,national.toChars());
    national.send();
  }else if(command.equals("HeatingThresholdTemperature")){
    national.setTemp(message.toInt());
    _HeatingThresholdTemperature=national.getTemp(); //may be capped
    client->publish(DEBUG,national.toChars());
    national.send();
  }else if(command.equals("RotationSpeed")){
    int speed = message.toInt();
    if     (speed < 25) national.setFan(kNationalFanAuto); //auto
    else if(speed < 50) national.setFan(kNationalFan1); //level-1
    else if(speed < 75) national.setFan(kNationalFan2); //level-2
    else                national.setFan(kNationalFan3); //level-3
    national.send();
  }else if(command.equals("SwingMode")){
    client->publish(DEBUG,"swing mode.");
    if(message.equalsIgnoreCase("DISABLED")) national.setSwing(false);
    if(message.equalsIgnoreCase("ENABLED")) national.setSwing(true);
    //national.send() is not required here, because setSwing() sends IR signal
  }else if(command.equals("restart") && message.equalsIgnoreCase("TRUE"))  
  { //reset ESP32 for debug
    client->publish(DEBUG,"Restarting ESP32..."); delay(500);
    //mainly used to retly mDNS and ArduinoOTA connection.
    ESP.restart();
  }
}


//message received from Z2MQTT (Flap sensor)
void onMessageReceivedF(const String& topic, const String& message) { 
  //message from Zigbee flap sensor
  client->publish(DEBUG,message); 
  DeserializationError error = deserializeJson(doc, message);
  if(error) {
    client->publish(DEBUG,"JSON deserialization error."); 
  }
  else if(doc["contact"] == true) {
    national.updateState( false );
    client->publish(PUBTOPICF,"false");
  }
  else if(doc["contact"] == false) {
    national.updateState( true );
    client->publish(PUBTOPICF,"true");
  }
}


void onConnectionEstablished() {
  ArduinoOTA.setHostname("irNational");
  ArduinoOTA.setPasswordHash("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
  ArduinoOTA.begin();
  Serial.println("MQTT connection established.");
  client->subscribe(SUBTOPICH, onMessageReceivedH); //set callback for HomeKit
  client->subscribe(SUBTOPICF, onMessageReceivedF); //for Z2MQTT (Flap sensor)
  client->publish(DEBUG,"irNational started.");
}


//IR Remo and MQTT: read DHT20 and publish results
//check every 10 sec.
//if temp or humi change publish soon, otherwise publish every 5 min.
void publishDHT() { 
  char buff[64];
  static int count10=0; //counts up in every 10 sec.
  float humi, temp;
  static int oldhumi=0, oldtemp=0; //previous value
  int newhumi, newtemp; //new value
  if(millis() - dht.lastRead() < 10000) return;//do nothing < 10 sec.
  count10++; //count up 10 s counter
  if(DHT20_OK != dht.read()){
    client->publish(DEBUG,"DHT20 Read Error.");
    return; //sensor error, do nothing.
  }
  //read the current temp and humi
  humi=dht.getHumidity();
  newhumi=round(humi);//int version
  temp=dht.getTemperature();
  newtemp=round(temp * 10);//int version (x10)
  //if measurement changes or 300 seconds passed
  if((oldtemp != newtemp) || (oldhumi != newhumi) || (count10 >= 30)){
    oldtemp=newtemp;
    oldhumi=newhumi;    
    sprintf(buff, "{\"temperature\":%.1f,\"humidity\":%.0f}", temp, humi);
    client->publish(PUBTOPICT,buff);
    count10=0; //reset counter
  }
}

void loop() {
  ArduinoOTA.handle(); //loop for OTA
  client->loop(); //loop for MQTT
  publishDHT(); //publish temp and humi if needed
}
