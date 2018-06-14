/*
- forked from bruhautomation MQTT on GitHub
- Using the following libraries to communicate with ESP8266 (-01)
      - PubSubClient
      - ArduinoJSON
 Function: - listen to a topic from an MQTT server
			- parse out JSON topic, and analogWrite light intensity 
 */
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

WiFiClient espClient;
PubSubClient client(espClient);
/*___________________SETUP WIFI, MQTT______________________*/
#define wifi_ssid "your_wifi" //type your WIFI information inside the quotes
#define wifi_password "your_password"
#define mqtt_server "ip_mqtt_server"
#define mqtt_user "username" 
#define mqtt_password "passowrd_mqtt"
#define mqtt_port 1883

/*___________________MQTT TOPIC (THREAD)______________________*/

#define light_state_topic "workingdesk/dimmerThree"
#define light_set_topic "workingdesk/Controller"

const char* on_cmd = "ON";
const char* off_cmd = "OFF";
#define SENSORNAME "dimmerThree"

/*___________________PIN MAPPING______________________*/
#define controlPin 2
#define ledPin 0

/*___________________OPERATING PARAMETERS______________________*/
char message_buff[300];
int calibrationTime = 0;
const int BUFFER_SIZE = 300;
float uptime;
int intensity = 0;
bool stateOn = false;
int state;



/*___________________SETUP______________________*/
void setup() {

  Serial.begin(115200);
  delay(1000);
  pinMode(controlPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
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

/*___________________SETUP WIFI______________________*/
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
/*___________________START CALLBACK MQTT______________________*/
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
/*___________________SPARSE JSON MESSAGE______________________*/
bool processJson(char* message) {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(message);
  Serial.print("Processing message:");
  Serial.println(message);
  
  if (!root.success()) {
    Serial.println("parseObject() failed");
    return false;
  }
  state = root["Three"]["state"];
  intensity = root["Three"]["intensity"];
  Serial.print("\tstate: ");
  Serial.print(state);
  Serial.print("\tintensity: ");
  Serial.println(intensity);
  if (state == 1){
    stateOn = true;
  } else {
    stateOn = false;
  }
  if (stateOn){
    analogWrite(controlPin, intensity);
    analogWrite(ledPin, intensity);
    Serial.print("\t>>  analogWrite:\t");
    Serial.print(intensity);
   } else {
    analogWrite(controlPin, 0);
    analogWrite(ledPin, 0);
  }
 return true;
}

/*___________________PUBLISH TO MQTT______________________*/
void sendState() {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();
  root["uptime"] = uptime;
  root["sensor"] = SENSORNAME;
  root["state"] = (stateOn) ? on_cmd : off_cmd;
  root["intensity"] = intensity;
  
  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));
  String sPayload = "";
  root.printTo(sPayload);
  char* cPayload = &sPayload[0u];

  client.publish(light_state_topic, buffer, true);
  Serial.println("\nPushed MQTT: " + String(buffer));
}

/*__________________RECONNECT _____________________*/
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(SENSORNAME, mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(light_set_topic);
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

/*___________________MAIN LOOP______________________*/
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

  
  uptime = millis()/1000/3600.00;
//  Serial.println(millis()/1000);
  delay(100);
}

/*___________________SOFT RESET______________________*/
void software_Reset() 
{
Serial.print("\tWill try reset");
delay(2000);
while (!client.connected()){
  Serial.print("\Reconnecting");
  delay(5000);
  ESP.reset(); 
  }; 
}

