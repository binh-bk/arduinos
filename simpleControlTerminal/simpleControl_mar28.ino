/*
 * CONTROL BOARD wiht 4 inputs, and an analog rotator for intensity
 * Binh Nguyen, Mar 25, 2018

*/

/*_________________________  LIBRARIES ________________________*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <SSD1306.h>
#include <SSD1306Wire.h>
#include <NTPClient.h>
#include <Time.h>
#include <TimeLib.h>
#include <Timezone.h>
/*_________________________  NTP CLOCKS ________________________*/
// Define NTP properties
#define NTP_OFFSET   60 * 60      // In seconds
#define NTP_INTERVAL 60 * 1000    // In miliseconds
#define NTP_ADDRESS  "asia.pool.ntp.org"  // change this to whatever pool is closest (see ntp.org)

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);

// Create a display object
SSD1306  display(0x3c, D1, D2); //0x3d for the Adafruit 1.3" OLED, 0x3C being the usual address of the OLED **C** for my model

/*_________________________  WIFI and MQTT INFORMATION (CHANGE THESE FOR YOUR SETUP) ________________________*/

const char* ssid = "wifi_SSID";   // insert your own ssid
const char* password = "wifi_password";              // and password
String date;
String t;
const char * days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"} ;
const char * months[] = {"Jan", "Feb", "Mar", "Apr", "May", "June", "July", "Aug", "Sep", "Oct", "Nov", "Dec"} ;
const char * ampm[] = {"AM", "PM"} ;

#define mqtt_server "mqtt_server"
#define mqtt_user "mqtt_user" 
#define mqtt_password "mqtt_password"
#define mqtt_port 1883
/*_________________________  MQTT TOPICS ________________________*/

#define light_state_topic "workingdesk/Controller"
#define light_set_topic "workingdesk/Controller/set"

const char* on_cmd = "ON";
const char* off_cmd = "OFF";

/*_________________________  FOR OTA ________________________*/

#define SENSORNAME "Controller"
#define OTApassword "OTA_password" // change this to whatever password you want to use when you upload OTA
int OTAport = 8266;

/*_________________________  PIN DEFINITIONS  ________________________*/

#define input D4
#define analogIn A0
int inputPins[] = {D5, D6, D7, D8};
String deviceName[] = {"One","Two", "Three", "Four"};
int deviceStatus[] = {0, 0, 0, 0};
bool s[] = {false, false, false, false};
int deviceSet[] = {0, 0, 0, 0};
unsigned int samplingDuration = 60;
unsigned long prevSample = 0;
unsigned int pubDuration = 300;  //5 minutes
unsigned long prevPub = 0;
String pinStatus = "", pinValues = "";
unsigned long uptime = 0;
int pinCounts = 4;
int diffDelta = 10;

char message_buff[100];
const int BUFFER_SIZE = 300;
bool stateOn = false;
bool willPublish = false;

/*_________________________  SENSOR DEFINITIONS  ________________________*/

WiFiClient espClient;
PubSubClient client(espClient);

/*_________________________  SETUP LOOP  ________________________*/

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(10);
  
  setup_wifi();
  
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  reconnect();
  Serial.print("MQTT passed");
  delay(2000);
  
  setupOTA();
  Serial.print("\tOTA passed");
  delay(2000);
    
  for (int i = 0; i < pinCounts; i++) {
    pinMode(inputPins[i], INPUT);
  }; 
  pinMode(analogIn, INPUT);
  pinMode(input, INPUT_PULLUP);
  Wire.begin(); // 0=sda, 2=scl
  Serial.print("\tExiting SETUP LOOP");
  delay(1000);
}
/*_________________________  SETUP OVER-AIR UPDATE(OTA)  ________________________*/

void setupOTA(){
  ArduinoOTA.setPort(OTAport);
  ArduinoOTA.setHostname(SENSORNAME);
  ArduinoOTA.setPassword((const char *)OTApassword);
  ArduinoOTA.onStart([]() {
    Serial.println("Starting");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  }

/*_________________________  START SETUP WIFI  ________________________*/

void setup_wifi() {

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  //Setup SSD1306 OLED
  display.init();
  display.flipScreenVertically();
  display.clear();
  display.drawString(0, 10, "Connecting to Wifi...");
  display.drawString(0, 24, "Connected.");
  display.display();
}
/*_________________________  START CALLBACK  ________________________*/

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

/*_________________________  START PROCESS JSON  ________________________*/

bool processJson(char* message) {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(message);

  if (!root.success()) {
    Serial.println("parseObject() failed");
    return false;
  }
  return true;
}
/*_________________________  START SEND STATE  ________________________*/

void sendState() {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();
  float uptime_hrs = uptime/3600.0;
  root["uptime"] = String(uptime_hrs);
  root["sensor"] = SENSORNAME;
  
  for (int i = 0; i < pinCounts; i++) {
    
    String device_name = deviceName[i];
    JsonObject& device = root.createNestedObject(device_name);
    device["state"] = deviceStatus[i];
    int val = deviceSet[i];
    device["intensity"] = deviceSet[i];
    }


  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));
  String sPayload = "";
  root.printTo(sPayload);
  char* cPayload = &sPayload[0u];

  client.publish(light_state_topic, buffer, true);
  Serial.println("TO MQTT: " + String(buffer));
}
/*_________________________  START RECONNECT  ________________________*/

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(SENSORNAME, mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(light_set_topic);
//      setColor(0, 0, 0);
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

/*_________________________ MAIN LOOP  ________________________*/

void loop() {

  ArduinoOTA.handle();
  if (!client.connected()) {
    reconnect();
    software_Reset();
  }
  client.loop();
  
  uptime = millis()/1000;
  int inputStatus = digitalRead(input);
  
  if (inputStatus == LOW){
    checkInputs();
  } else{
    bool willPublish =  s[0] || s[1] || s[2] || s[3];
    if (isTimePublish() || willPublish){
      sendState();
      s[0] = s[1] = s[2]= s[3] = false;
    }
    //running to clock
    if ((uptime - prevSample) > samplingDuration){
      //when buton is NOT pressed, and the 60 seconds since the last update, update the time
      prevSample = uptime;
      displayClock();
    }
  }
  Serial.print("\nUptime:");
  Serial.print(uptime);
  Serial.print("\tlastPub:");
  Serial.print(prevPub);
  Serial.print("\tlastSampling:");
  Serial.print(prevSample);
  Serial.print("\tD3 status:");
  Serial.print(inputStatus);
//  Serial.print("\ts:");
//  Serial.print(s);
  Serial.print("\twillPublish:");
  Serial.println(willPublish);
  delay(1000);
}

/*_________________________ CHECK INPUTS   ________________________*/

int checkInputs(){
  

  for (int i = 0; i < pinCounts; i++) {
    int pinRead = digitalRead(inputPins[i]);
    
    if (pinRead == HIGH){
      unsigned int val = analogRead(analogIn);
      unsigned int _val = deviceSet[i];
      if (isNewInput(val, _val)){
        deviceSet[i] = val;
        int onState = (val > 100)? 1: 0;  //change this value to set the state ON - OFF
        deviceStatus[i] = onState;
        s[i] = true;
        Serial.print("\tNew Value");
        Serial.print(isNewInput(val, _val));
      }
      Serial.print("\tA0 Value");
      Serial.print(val);
    }
    delay(100);  
  }
    pinValues = String(deviceSet[0]) + ":"+ String(deviceSet[1]) \
    +":"+ String(deviceSet[2]) + ":"+ String(deviceSet[3]);
    pinStatus = String(deviceStatus[0]) + ":"+ String(deviceStatus[1]) \
    + ":" +String(deviceStatus[2]) + ":"+ String(deviceStatus[3]);

    Serial.print("\tD5-D8 Status:");
    Serial.print(pinStatus);
    Serial.print("\tD5-D8 Set:");
    Serial.println(pinValues);
    delay(100);
    
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawStringMaxWidth(64, 0, 128, pinStatus);
    display.setFont(ArialMT_Plain_10);
    display.drawStringMaxWidth(64, 40, 128, pinValues);
    display.display();
}
/*_________________________ SIMPLE CLOCK   ________________________*/


void displayClock(){
  if (WiFi.status() != WL_CONNECTED) //if WIFI is not connected
  {
    display.clear();
    display.drawString(0, 10, "Connecting to Wifi...");
    WiFi.begin(ssid, password);
    display.drawString(0, 24, "Connected.");
    display.display();
    delay(100);
  }
   //proceed to get the data
   
    date = "";  // clear the variables
    t = "";

    // update the NTP client and get the UNIX UTC timestamp
    timeClient.update();
    unsigned long epochTime =  timeClient.getEpochTime();

    // convert received time stamp to time_t object
    time_t local, utc;
    utc = epochTime;

    // Then convert the UTC UNIX timestamp to local time
    TimeChangeRule usEDT = {"EDT", Second, Sun, Mar, 2, +360};  //UTC +7  hours - change this as needed
    TimeChangeRule hanoi = {"EST", First, Sun, Nov, 2, +360};   //UTC + 7 hours - change this as needed
    Timezone vnHanoi(usEDT, usEDT);
    local = vnHanoi.toLocal(utc);

    // now format the Time variables into strings with proper names for month, day etc
    date += days[weekday(local) - 1];
    date += ", ";
    date += months[month(local) - 1];
    date += " ";
    date += day(local);
    date += ", ";
    date += year(local);

    // format the time to 12-hour format with AM/PM and no seconds
    t += hourFormat12(local);
    t += ":";
    if (minute(local) < 10) // add a zero if minute is under 10
      t += "0";
    t += minute(local);
    t += " ";
    t += ampm[isPM(local)];

    // Display the date and time
    Serial.println("");
    Serial.print("Local date: ");
    Serial.print(date);
    Serial.print("\tLocal time: ");
    Serial.println(t);

    // print the date and time on the OLED
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.setFont(ArialMT_Plain_24);
    display.drawStringMaxWidth(64, 10, 128, t);
    display.setFont(ArialMT_Plain_10);
    display.drawStringMaxWidth(64, 38, 128, date);
    display.display();
    delay(100);
}

/*_________________________ BOUND READING   ________________________*/

bool isNewInput(int _old, int _new){
  if (((_new - _old) > diffDelta) || ((_old - _new) > diffDelta)) {
    return true;
      }
  return false;
}

/*_________________________ RESET  ________________________*/

void software_Reset() // Restarts program from beginning but does not reset the peripherals and registers
{
  delay(2000);
  if (!client.connected()){
    Serial.print("\Resetting");
    ESP.reset(); 
  }
}

bool isTimePublish(){
  if ((uptime - prevPub)> pubDuration){
    prevPub = uptime;
    return true;
  }
  return false;
}
