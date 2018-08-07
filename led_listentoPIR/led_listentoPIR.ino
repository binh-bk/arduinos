/*
- forked from: bruhautomation on github:
- using mqtt to publish and subscribe to MQTT and JSON format
      - PubSubClient
      - ArduinoJSON
  Binh Nguyen, July 06, 2018
*/
/*___________________________ LIBRARIES _______________________________________*/
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

WiFiClient espClient;
PubSubClient client(espClient);
/*___________________________ WIFI, MQTT _______________________________________*/
#define wifi_ssid "wifi_ssid" //type your WIFI information inside the quotes
#define wifi_password "wifi_password"
#define mqtt_server "192.168.1.50"
#define mqtt_user "janedoe" 
#define mqtt_password "johndoe"
#define mqtt_port 1883
#define subscribe_topic "sensor/door/pir"
#define SENSORNAME "One1W"

const char* on_cmd = "ON";
const char* off_cmd = "OFF";

/*___________________________ PIN MAP _______________________________________*/
#define controlPin 2 //GPIO1
#define ledPin 0 //GPIO1

/*___________________________ GLOBAL VARIABLES_______________________________________*/
char message_buff[300];
int calibrationTime = 0;
#define DELAYS  12 //delay 12 second after the PIR sensor OFF
const int BUFFER_SIZE = 300;
float uptime;
int brightness = 0;
bool stateOn = false;
int state;

/*___________________________ RECONNECT _______________________________________*/
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(SENSORNAME, mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(subscribe_topic);
//      setColor(0);
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

/*___________________________ SETUP LOOP _______________________________________*/
void setup() {

  Serial.begin(115200);
  delay(1000);
  pinMode(controlPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  Serial.println("Starting Node named " + String(SENSORNAME));

  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  reconnect();
}

/*___________________________ SEND STATE _______________________________________*/
void sendState() {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();
  root["uptime"] = uptime;
  root["sensor"] = SENSORNAME;
  root["state"] = (brightness >=100) ? on_cmd : off_cmd;
  root["intensity"] = brightness;
  
  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));
  String sPayload = "";
  root.printTo(sPayload);
  char* cPayload = &sPayload[0u];

  client.publish(publish_topic, buffer, true);
  Serial.println("\nPushed MQTT: " + String(buffer));
}

/*___________________________ SETUP WIFI _______________________________________*/
void setup_wifi() {

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    i++;
    Serial.printf("..%d..", i);
    if ((i%10)==0){
      WiFi.begin(wifi_ssid, wifi_password);
    }
    else if (i==29){
      Serial.println("Cannot conect to router, restarting now...");
      ESP.restart();
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

/*___________________________ PROCESS JSON _______________________________________*/
bool processJson(char* message) {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(message);
  Serial.print("Processing message:");
  Serial.println(message);
  
  if (!root.success()) {
    Serial.println("parseObject() failed");
    return false;
  }
  if(root.containsKey("motion")){
   if (root["motion"] == "1"){
    stateOn = 1;
   } else {
    stateOn = 0;
   }
  }
 return true;
}

/*___________________________ CALLBACK _______________________________________*/
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


/*___________________________ MAIN LOOP _______________________________________*/
void loop() {
  
  if (!client.connected()) {
      reconnect();
      delay(3000);
      }
    if (!client.connected()){
      Serial.print("\n: Restarting the ESP");
      software_Reset();
    }
  client.loop();
  setLeds(DELAYS);

  
  uptime = millis()/1000/3600.00;
//  Serial.println(millis()/1000);
  delay(1000);
}
/*___________________________ RESET _______________________________________*/
void software_Reset() // Restarts program from beginning but does not reset the peripherals and registers
{
Serial.print("\tWill try reset");
delay(2000);
while (!client.connected()){
  Serial.print("\Reconnecting");
  delay(5000);
  ESP.reset(); 
  }; 
}

/*___________________________ SET LEDs _______________________________________*/

void setLeds(int onSeconds){
    float prev = 0;
    if (stateOn==1){
    brightness = 1000;
    analogWrite(controlPin, brightness);
    analogWrite(ledPin, brightness);
    Serial.print("\t>>  analogWrite:\t");
    Serial.print(brightness);
    for (int i=0; i<onSeconds; i++){
      delay(1000);
      client.loop();
      if ((stateOn==1) && (millis()/1000 - prev > 10)) {
        prev = millis()/1000;
        i=0;
      }
      Serial.printf("\nstateON is %i \t", stateOn);
      Serial.printf("\t Counting on i: %d \n", i);
    }
   } else {
    brightness = 0;
    analogWrite(controlPin, brightness);
    analogWrite(ledPin, brightness);
  }
}

