/*
Binh Nguyen, August 7, 2018
*/
/*________________________ INCLUDE LIBRARIES  _____________________________*/
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
WiFiClient espClient;
PubSubClient client(espClient);

/*________________________ WIFI and MQTT INFORMATION  _____________________________*/
#define wifi_ssid "wif_ssid"
#define wifi_password "wifi_passowrd"
#define mqtt_server "192.168.1.50"
#define mqtt_user "janedoe" 
#define mqtt_password "johndoe"
#define mqtt_port 1883

/*________________________ MQTT TOPICS   _____________________________*/
#define light_state_topic "sensor/door/pir"
#define SENSORNAME "pirOne"
const char* on_cmd = "ON";
const char* off_cmd = "OFF";

/*________________________ PIN DEFINITIONS  _____________________________*/
#define PIR_SENSOR_PIN 0 
#define LED_PIN  2    

/*________________________ GLOBAL VARIABLES  _____________________________*/
char message_buff[100];
int calibrationTime = 0;
const int BUFFER_SIZE = 300;
float uptime;
unsigned long preSampling = 0;
int pirState;
String motionStatus;
bool stateOn = false;

/*________________________ START SETUP  _____________________________*/
void setup() {

  delay(100);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(PIR_SENSOR_PIN, INPUT);
  Serial.begin(115200);
  delay(10);
  Serial.println("Starting Node named " + String(SENSORNAME));

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  Serial.println("Ready");
  Serial.print("IPess: ");
  Serial.println(WiFi.localIP());
  reconnect();
}

/*________________________ START SETUP WIFI _____________________________*/
void setup_wifi() {

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);

  int i = 0;
  while (WiFi.status()!= WL_CONNECTED){
    delay(1000);
    i++;
    Serial.printf(" %i ", i);
    if ((i % 10)==0){
      WiFi.begin(wifi_ssid, wifi_password);
      delay(1000);
    }
    
    if (i >=30){
      Serial.printf("Resetting ESP");
      ESP.restart();
     }
    }
  Serial.println("WiFi connected, IP address: ");
  Serial.println(WiFi.localIP());
}

/*________________________ START CALLBACK  _____________________________*/
void callback(char* topic, byte* payload, unsigned int length) {
//  Serial.print("Message arrived [");
//  Serial.print(topic);
//  Serial.print("] ");

  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
//  Serial.println(message);

  if (!processJson(message)) {
    return;
  }
  //map the value to LED if necessary
  if (stateOn && (pirState==0)) {
    digitalWrite(LED_PIN, HIGH);
  } else if (!stateOn) digitalWrite(LED_PIN, LOW);

  sendState();
}

/*________________________ START PROCESS JSON  _____________________________*/
bool processJson(char* message) {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(message);

  if (!root.success()) {
    Serial.println("parseObject() failed");
    return false;
  }
   if (root.containsKey("state")) {
    if (strcmp(root["state"], on_cmd) == 0) {
      stateOn = true;
    }
    else if (strcmp(root["state"], off_cmd) == 0) {
      stateOn = false;
    }
  };
  return true;
}

/*________________________ START SEND STATE _____________________________*/
void sendState() {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  uptime = millis()/1000/3600.00;
  root["uptime"] = (String)uptime;
  root["sensor"] = SENSORNAME;
  root["state"] = (stateOn) ? on_cmd : off_cmd;
  root["motion"] = (String)pirState;

  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));
  String sPayload = "";
  root.printTo(sPayload);
  char* cPayload = &sPayload[0u];

  client.publish(light_state_topic, buffer, true);
  
//  Serial.println("Published 1: " + String(buffer));

}
/*________________________ START RECONNECT  _____________________________*/
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
//    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(SENSORNAME, mqtt_user, mqtt_password)) {
      Serial.println("connected");
//      client.subscribe(light_set_topic);
       sendState();
    } else {
//      Serial.print("failed, rc=");
//      Serial.print(client.state());
//      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

/*________________________ START MAIN LOOP _____________________________*/
void loop() {
  
  if (!client.connected()) {
    // reconnect();
    software_Reset();
  }
  client.loop();
  bool pir = digitalRead(PIR_SENSOR_PIN);
  
//  Serial.print("PIR read: ");
//  Serial.println(pir);
  if (pir==HIGH && pirState !=1){
    motionStatus = "motion detected";
    pirState=1;
    sendState();
  } else if (pir==LOW && pirState !=0){
    motionStatus = "standby";
    pirState=0;
    sendState();
  }
  if (pir==HIGH){
    digitalWrite(LED_PIN, HIGH);
  } else digitalWrite(LED_PIN, LOW);
  delay(100);
  yield();
}

/*________________________ RESET  _____________________________*/
void software_Reset(){
  Serial.print("resetting");
  ESP.reset(); 
}


