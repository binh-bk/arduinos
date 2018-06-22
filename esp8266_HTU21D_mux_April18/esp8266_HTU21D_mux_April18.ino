/* ESP8266 with MULTIPLEX (or MUX) using CD4051BE
 *  Binh Nguyen, April 14, 2018
 * sensors including: analog: photosensor, temperature, 
 * light intensity: BH1750
 * high precision humid
 * 18B20
 * OTA
 */

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

/*_____________________WIFI and MQTT INFORMATION (CHANGE THESE FOR YOUR SETUP)_______________________*/
#define wifi_ssid "openwifi"    //
#define wifi_password "11223344"
#define mqtt_server "192.168.1.100"
#define mqtt_user "username" 
#define mqtt_password "12341234"
#define mqtt_port 1883

WiFiClient espClient;
PubSubClient client(espClient);

/*_____________________MQTT TOPICS_______________________*/
#define light_state_topic "workingdesk/esp12"
#define light_set_topic "workingdesk/Controller"

const char* on_cmd = "ON";
const char* off_cmd = "OFF";

/*_____________________FOR OTA_______________________*/
#define SENSORNAME "esp12"
#define OTApassword "112123" // OTA password can be in hashed form
int OTAport = 82666;

/*_____________________PIN DEFINITIONS_______________________*/
#define analogPin A0   //photoresistor, D4 (GPIO2) attached to LED_BUILDIN, on with sink the pin
//I2C: D1--CLK; D2--SDA, 
//open D0, D3
#define YELLOW_LED D0
#define Temp18B20Pin D3
#define LED_BUILDIN D4


#define MUX_A D6
#define MUX_B D7
#define MUX_C D8

/*_____________________OPERATONAL PARAMETERS_______________________*/
int photoValue = 0;
int diffPhoto = 20;

uint16_t luxValue;
int diffLUX = 100;
float tempBMPValue;
float diffTEMP = 2;
int pressureValue;
int diffPRESS = 100;
float altitudeValue;
float temp1820Value;
float humidValue, tempValue;
int diffHumid =1;
int lm35Value;
int lm35inC;
int tem6000Value;

int samplingTimes = 5;
unsigned long preSampling = 0;


int samplingInterval = 600; //60 seconds
char message_buff[100];
int calibrationTime = 0;

const int BUFFER_SIZE = 300;
float uptime = 0;

byte intensity = 100;
bool stateOn = false;

/*_____________________LIBRARY & SENSOR_______________________*/
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <BH1750.h>
#include <Adafruit_BMP085.h>
#include "SparkFunHTU21D.h"

Adafruit_BMP085 bmp;
OneWire oneWire(Temp18B20Pin);
DallasTemperature tempSensor(&oneWire); 
BH1750 lightMeter;
HTU21D myHumidity;

/*_____________________LIBRARY & SENSOR_______________________*/
int muxPin[] = {0, 2, 3};
static const int photoPin = 0;
static const int lm35Pin = 2;
static const int tem6000Pin = 3;

/*_____________________SETUP LOOP_______________________*/
void setup() {
  Serial.begin(115200);
  Serial.println("Booting");
  pinMode(LED_BUILDIN, OUTPUT);
  pinMode(photoPin, INPUT);
  pinMode(Temp18B20Pin, INPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(MUX_A, OUTPUT);
  pinMode(MUX_B, OUTPUT);     
  pinMode(MUX_C, OUTPUT); 
   
  Wire.begin();
  lightMeter.begin();
  bmp.begin();
  
  if (!bmp.begin()) {
    Serial.println("BMP085 not found, check wiring!");
//    while (1) {}
  }
  tempSensor.setWaitForConversion(false); // Don't block the program while the temperature sensor is reading
  tempSensor.begin();                     // Start the temperature sensor
  if (tempSensor.getDeviceCount() == 0) {
    Serial.printf("No DS18x20 found on pin %d.", Temp18B20Pin);
    Serial.flush();
    delay(10000);
//    ESP.reset();
 }
  myHumidity.begin();
  Serial.print("calibrating sensor ");
  for (int i = 0; i < calibrationTime; i++) {
    Serial.print(".");
    delay(1000);
  }

  setup_wifi();
  setup_OTA();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  reconnect();
}
/*_____________________MAIN LOOP_______________________*/
void loop() {
  ArduinoOTA.handle();
  if (!client.connected()) {
    reconnect();
    delay(1000);
    if (!client.connected()) software_Reset();
  }
  client.loop();
  
  uptime = millis()/1000/3600.00;

  int newPhotoValue = analog_read(photoPin, samplingTimes);  
  if (checkBoundSensor(newPhotoValue, photoValue, diffPhoto)){
    photoValue = newPhotoValue;
    Serial.println("LDR:  ");
    Serial.println(photoValue);
    sendState();
    }

   int newTem6000Value = analog_read(tem6000Pin, samplingTimes);  
  if (checkBoundSensor(newTem6000Value, tem6000Value, diffPhoto)){
    tem6000Value = newTem6000Value;
    Serial.println("TEM6000:  ");
    Serial.println(tem6000Value);
    sendState();
    }

  uint16_t newluxValue = lightMeter.readLightLevel();
  if (checkBoundSensor(newluxValue, luxValue, diffLUX)) {
    
      luxValue = newluxValue;
      Serial.println("BH1750:  ");
      Serial.print(luxValue);
      sendState();
    }
    float newTempBMPValue = bmp.readTemperature();
    float newPressureValue = bmp.readPressure();
    altitudeValue = bmp.readAltitude();
    if (checkBoundSensor(newTempBMPValue, tempBMPValue, diffTEMP)){
      tempBMPValue = newTempBMPValue;
      Serial.println("BMP-temp:  ");
      Serial.print(tempBMPValue);
      sendState();
    }
    if (checkBoundSensor(newPressureValue, pressureValue, diffPRESS)) {
      pressureValue = newPressureValue;
      Serial.println("BMP-press:  ");
      Serial.print(pressureValue);
      sendState();
    }

  tempSensor.requestTemperatures();
  float newTemp1820Value = tempSensor.getTempCByIndex(0);
  if (checkBoundSensor(newTemp1820Value, temp1820Value, diffTEMP)){
      temp1820Value = newTemp1820Value;
      Serial.println("18B20:  ");
      Serial.print(temp1820Value);
      sendState();
    }

  float newHumid = myHumidity.readHumidity();
  float newTemp = myHumidity.readTemperature();
  if (checkBoundSensor(newHumid, humidValue, diffHumid)){
      humidValue = newHumid;
      Serial.println("HTU-Humid:  ");
      Serial.print(humidValue);
      sendState();
    }
  if (checkBoundSensor(newTemp, tempValue, diffTEMP)){
      tempValue = newTemp;
      Serial.println("HTU-temp:  ");
      Serial.print(tempValue);
      sendState();
    }

   int newLM35Value = analog_read(lm35Pin, samplingTimes);  
  if (checkBoundSensor(newLM35Value, lm35Value, diffPhoto)){
    lm35Value = newLM35Value;
    Serial.println("LM35 Raw:  :");
    Serial.print(lm35Value);
    lm35inC = (lm35Value/1024)*330;
    
    Serial.println("  LM35:  :");
    Serial.print(lm35inC);
    sendState();
    }

  if (neededSample(samplingInterval)){
    sendState();  
  }
  delay(1000);
}

/*_____________________SETUP WIFI_______________________*/
void setup_wifi(){
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

/*_____________________SETUP OTA_______________________*/
void setup_OTA(){
  // Port defaults to 3232
   ArduinoOTA.setPort(OTAport);

  // Hostname defaults to esp3232-[MAC]
   ArduinoOTA.setHostname(SENSORNAME);

  // No authentication by default
   ArduinoOTA.setPassword((const char *)OTApassword);
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  ArduinoOTA.setPasswordHash((const char *)OTApassword);

  ArduinoOTA.onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
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

/*_____________________START CALLBACK_______________________*/
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
/*_____________________START PROCESS JSON_______________________*/
bool processJson(char* message) {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.parseObject(message);

  if (!root.success()) {
    Serial.println("parseObject() failed");
    return false;
  };

  int state = root["Four"]["state"];
  int intensity = root["Four"]["intensity"];
  Serial.print("\tstate: ");
  Serial.print(state);
  Serial.print("\tintensity: ");
  Serial.println(intensity);
  if (state == 1){
    stateOn = true;
  } else {
    stateOn = false;
  }
 setLED(stateOn, intensity);

 return true;
}

/*_____________________START SEND STATE_______________________*/
void sendState() {
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();

//  root["state"] = (stateOn) ? on_cmd : off_cmd;

//  root["brightness"] = brightness;
  root["uptime"] = uptime;
  root["sensor"] = SENSORNAME;
  root["ldr"] = photoValue;
  root["tem6000"] = tem6000Value;
  root["BH1780"] = luxValue;
  root["BMPtemp"] = tempBMPValue;
  root["BMPpress"] = pressureValue;
  root["Altitude"] = altitudeValue;
  root["18B20"] = temp1820Value;
  root["lm35"] = lm35inC;
  root["HTU21DTemp"] = tempValue;
  root["HTU21DHumid"] = humidValue;
  root["HeatIndex"] = heatIndex(tempValue, humidValue);

  char buffer[root.measureLength() + 1];
  root.printTo(buffer, sizeof(buffer));
  String sPayload = "";
  root.printTo(sPayload);
  char* cPayload = &sPayload[0u];

  client.publish(light_state_topic, buffer, true);
  Serial.println("\nPublished MQTT: " + String(buffer));
}

/*_____________________START RECONNECT_______________________*/

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(SENSORNAME, mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(light_set_topic);
//      ledStatus(stateOn);
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
/*_____________________SOFT RESET_______________________*/
void software_Reset() // Restarts program from beginning but does not reset the peripherals and registers
{
  Serial.print("resetting");
  ESP.restart();
}


/*_____________________SAMPLING INTERVAL_______________________*/
bool neededSample(int intervalSampling){
  unsigned long currentTime = millis();
  intervalSampling *=1000;
  if ((currentTime- preSampling) > intervalSampling){
    preSampling = currentTime;
    return true;
  } else return false;
  
}
/*_____________________START CHECK SENSOR_______________________*/
bool checkBoundSensor(float newValue, float prevValue, float maxDiff) {
  return newValue < prevValue - maxDiff || newValue > prevValue + maxDiff;
}

/*_____________________MULTIPLE ANALOG READS_______________________*/

int analog_read(int pin2Read, int samplingTimes){
  changeMux(pin2Read);
  delay(100);
  int sum = 0;
  for (int i=0; i<samplingTimes; i++){
    sum += analogRead(analogPin);
    delay(100);
  }
  return sum/samplingTimes;
 }

 /*_____________________SET LEDS_______________________*/
 void setLED(bool stateOn, int intensity){
//  dutyCycle = map(potentiometerValue, 0, 4095, 0, 255);
  if (stateOn){
    analogWrite(LED_BUILDIN, intensity);
    analogWrite(YELLOW_LED, intensity);
  } else {
     analogWrite(LED_BUILDIN, 0);
     analogWrite(YELLOW_LED, 0);
  }
 }
/*_____________________CONVERT HEATINDEX_______________________*/
float heatIndex(float Tc, float RH){
float T = Tc*9/5 + 32;
float HI = -42.379 + 2.04901523*T + 10.14333127*RH - 0.22475541*T*RH \
  - 0.00683783*T*T - 0.05481717*RH*RH + 0.00122874*T*T*RH \
  + 0.00085282*T*RH*RH - 0.00000199*T*T*RH*RH;
return (HI - 32)*5/9;
}
/*_____________________CONTROL MULTIPLEX CD4051_______________________*/
void changeMux(int input) {
    int a = input%2;
    int b = (input/2)%2;
    int c = (input/4)%2;
//    Serial.printf("\n in DEC %d and in BINARY c-b-a as %d,%d,%d\t", input, c, b, a);
    delay(100);
  
  digitalWrite(MUX_A, a);
  digitalWrite(MUX_B, b);
  digitalWrite(MUX_C, c);
}
/*_____________________SETUP OTHERS_______________________*/

