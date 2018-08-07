/*
forked from: bruhautomation multisensor on github.ino
- using mqtt and JSON format
      - PubSubClient
      - ArduinoJSON
      
Changes by Binh Nguyen from bruhautomation file, updated on May 07, 2018:
- trimed down setColor function.  Only analogWrite 
- trimed down OTA update
- added back pir to turn to LED
- keep LED on after the last HIGH detected from PIR (30 secs, can be adjusted)
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

/************ WIFI and MQTT INFORMATION (CHANGE THESE FOR YOUR SETUP) ******************/
#define wifi_ssid "your_id_wifi" //type your WIFI information inside the quotes
#define wifi_password "your_wifi_password"
#define mqtt_server "your_ip_mqtt_server"
#define mqtt_user "your_mqtt_username" 
#define mqtt_password "your_mqtt_password"
#define mqtt_port 1883

/************* MQTT TOPICS (change these topics as you wish)  **************************/
#define light_state_topic "livingroom/kitchenLight"
#define light_set_topic "workingdesk/Controller"

const char* on_cmd = "ON";
const char* off_cmd = "OFF";

/**************************** FOR OTA **************************************************/
#define SENSORNAME "kitchenLight"
/**************************** PIN DEFINITIONS ********************************************/
#define outPin 0
#define pirPin 2
int ledPin = 3;
//#define outPin D7
//#define pirPin D5

/**************************** SENSOR DEFINITIONS *******************************************/
char message_buff[300];
int calibrationTime = 0;

const int BUFFER_SIZE = 300;
float uptime;
unsigned long prevTime = 0;
/******************************** GLOBALS for fade/flash *******************************/
int intensity = 900;
int lightState = 0;
int pirRead = 0;
unsigned int onRetain = 30; //in seconds after the last movement detected

WiFiClient espClient;
PubSubClient client(espClient);

/********************************** START SETUP*****************************************/
void setup() {
  Serial.begin(115200);
  flash(ledPin, 5, 100);
  delay(1000);
  pinMode(outPin, OUTPUT);
  pinMode(ledPin, FUNCTION_3);  //to use this pin as RX pin, change to FUNCTION_3
  pinMode(ledPin, OUTPUT);
  pinMode(pirPin, INPUT);
  Serial.begin(115200);
  delay(10);
  Serial.println("Starting Node named " + String(SENSORNAME));
  Serial.println("Updated on: May 16, 2018");

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  Serial.println("Ready");
  Serial.print("IPess: ");
  Serial.println(WiFi.localIP());
  reconnect();
}

/********************************** START SETUP WIFI*****************************************/
void setup_wifi() {

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

/********************************** START CALLBACK*****************************************/
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  Serial.println(message);

  if (!processJson(message)) {
    return;
  }
  sendState();
}

/********************************** START PROCESS JSON*****************************************/
bool processJson(char* message) {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(message);
  Serial.print("Processing message:");
  Serial.println(message);
   
  if (!root.success()) {
    Serial.println("parseObject() failed");
    return false;
  }
  if (pirRead == 0){             //if motion is not detected
    lightState = root["Two"]["state"];
    intensity = root["Two"]["intensity"];
    Serial.print("\tLight state: ");
    Serial.print(lightState);
    Serial.print("\tintensity: ");
    Serial.println(intensity);
  } else {
    lightState = 1;
    intensity = 900;    //any values less than 1024 would work
  }
 return true;
}

/********************************** START SEND STATE*****************************************/
void sendState() {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();
  root["uptime"] = uptime;
  root["sensor"] = SENSORNAME;
  root["state"] = (lightState == 1) ? on_cmd : off_cmd;
  root["intensity"] = intensity;
  root["pir"] = pirRead;
  
  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));
  String sPayload = "";
  root.printTo(sPayload);
  char* cPayload = &sPayload[0u];
  client.publish(light_state_topic, buffer, true);
  Serial.println("\nPushed MQTT: " + String(buffer));
}

/********************************** START RECONNECT*****************************************/
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(SENSORNAME, mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(light_set_topic);
      sendState();
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

/********************************** START MAIN LOOP***************************************/
void loop() {
  
  if (!client.connected()) {
      reconnect();
      delay(3000);
      }
    if (!client.connected()){
      Serial.print("\n: Restarting the ESP");
      software_Reset();
    }
//  client.loop();
 
  uptime = millis()/1000/3600.00;
  pirRead = digitalRead(pirPin);
//  Serial.print("\tpirRead: ");
//  Serial.print(pirRead);
//  
  if (pirRead == 1){
    if (lightState == 0){
      lightState = 1;
      intensity = 900;
      send2LEDs();
      sendState();
    }
  } else {  //pirRead is zero
    if (lightState == 1){
       for (int i=0; i < onRetain; i++){
        pirRead = digitalRead(pirPin);
        if (pirRead == 1) {
          i= 0;
        };
        flashPattern(i, ledPin);
        delay(1000);
       }       
       lightState = 0;
        send2LEDs();
        sendState();
      } else {
        client.loop();
        send2LEDs();
    } 
  }
//  Serial.print("\tLightState: ");
//  Serial.print(lightState);
//  Serial.print("\n");
  delay(100);
}

/*_________________flash pattern________________________________*/

void flashPattern(int i, int ledPin){
  if (i=0) {
    flash(ledPin, 2, 100);
  };
  if (i= (onRetain-2)) {
    flash(ledPin, 3, 100);
  }
}
void flash(int ledPin, int times, int interval){
  for (int j=0; j < times; j++){
    digitalWrite(ledPin, HIGH);
    delay(interval);
    digitalWrite(ledPin, LOW);
    delay(interval);
  }
}
/*_________________WRITE OUTPUT TO LEDS________________________________*/
void send2LEDs(){
  if (lightState == 1){
    analogWrite(outPin, intensity);
    Serial.print("\t>>  analogWrite:\t");
    Serial.print(intensity);
  } else {
    analogWrite(outPin, 0);
  }
}

/*_________________UPDATE STATUS TO MQTT________________________________*/

bool isStateChange(int lightState, int  pirRead){
  if (((pirRead == 1) && (lightState == 0)) ||  ((pirRead == 0) && (lightState == 1))) {     //if digitalRead pir Pin is HIGH, and the light is OFF
    return true;
  }
}

/*_________________SOFT RESET________________________________*/
void software_Reset() {
Serial.print("\tWill try reset");
delay(2000);
while (!client.connected()){
  Serial.print("\Reconnecting");
  delay(5000);
  ESP.reset(); 
  }; 
}
