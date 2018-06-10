/*
 * based on Adafruit_PCD8544.h, INA219 libraries
 * NOKIA b&w LCD screeen 5110
 * INA219 current sensor
 * TSL2561 luminosity sensor
 * photoresistor 
 * 18B20 temperature sensor
 * By Binh Nguyen, May 10, 2018
 */
 

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define ldr A1
#define TEMP_BUS 9
#define ledButton 10
#define ledGND 2
#define samplingFreq  50
int interval = 0;
unsigned long currentSecond = 0;


OneWire temp(TEMP_BUS);
DallasTemperature t_sensor(&temp);

// Software SPI (slower updates, more flexible pin options):
// pin 4 - Serial clock out (SCLK) pin 13 if hardware SPI used
// pin 5  - Serial data out (DIN)  pin 11 if hardware SPI used
// pin 6  - Data/Command select (D/C)
// pin 7 - LCD chip select (CS)
// pin 8- LCD reset (RST)
Adafruit_PCD8544 display = Adafruit_PCD8544(4, 5, 6, 7, 8);  //Adafruit_PCD8544(5, 4, 3);

Adafruit_INA219 ina219;

unsigned long previousSecond = 0;
float current_mA = 0;
float loadvoltage = 0;
float current_avg;
float voltage_avg;
float energy = 0;
byte id;

Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);
uint16_t luxValue = 0;
int ldrValue = 0;
float tempValue = 0;
bool ledStatus = false;



void setup() {
  Serial.begin(115200);
  Serial.println("Starting...");
  pinMode(ldr, INPUT);
  pinMode(ledButton, INPUT_PULLUP);
  pinMode(ledGND, OUTPUT);
  Wire.begin();
  tsl.begin();
  tsl.enableAutoRange(true);  
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);
  ina219.begin();
  t_sensor.begin();
  t_sensor.setResolution(9);
  
  display.begin();
  display.setContrast(50);
  display.setRotation(1);
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);
  display.println("Start...");
  display.display();
  delay(2000);
  interval = 1000/samplingFreq;

  String header =  "";
  header = String("sec") + "," +"V" + "," + "mA" + "," + "mW" \
    + "," + "-" + "," + "lux" +  "," + "oC";
  Serial.println(header);
  delay(2000);
}

void loop() {
  currentSecond = millis()/1000;
  current_avg = 0;
  voltage_avg = 0;
  
  ina219reads();
  sensors_event_t event;
  tsl.getEvent(&event);
  luxValue = event.light;
  t_sensor.requestTemperatures();
  tempValue = t_sensor.getTempCByIndex(0);
  ldrValue = analogRead(ldr);
   
  displaydata();

  if (digitalRead(ledButton) == 0) {
    ledStatus = !ledStatus;
  }
  
  if (ledStatus){
    digitalWrite(ledGND, LOW);
  } else digitalWrite(ledGND, HIGH);

  if (currentSecond - previousSecond >= 10)  {
    previousSecond = currentSecond;
    String buff  = String(currentSecond) + "," + String(voltage_avg) + "," \
    + String(current_avg) + "," + String(energy) +"," + String(ldrValue) + "," \
    + String(luxValue) + "," + String(tempValue);
    Serial.println(buff);
    buff = "";
  }
}

void displaydata() {
  display.clearDisplay();
  display.setTextColor(BLACK);
  display.setTextSize(1);
  display.setCursor(0, 4);
  timeConvert(currentSecond);
  display.setCursor(0, 14);
  display.print("mA");
  display.setCursor(20, 14);
  display.print(current_avg, 0);
  display.setCursor(0, 24);
  display.print("V");
  display.setCursor(20, 24);
  display.print(voltage_avg, 1);
  display.setCursor(0, 34);
  display.print("mW");
  display.setCursor(20, 34);
  display.print(voltage_avg * current_avg, 0);
  display.setCursor(0, 44);
  display.print("mWh");
  display.setCursor(20, 44);
  display.print(energy, 0);
  display.setCursor(0, 54);
  display.print("oC");
  display.setCursor(20, 54);
  display.print(tempValue,1);
  display.setCursor(0, 64);
  display.print("ldr");
  display.setCursor(20, 64);
  display.print(ldrValue);
  display.setCursor(0, 74);
  display.print("lux");
  display.setCursor(20, 74);
  display.print(luxValue);

  display.display();
}

void ina219reads(){
  for (int i=0; i<samplingFreq; i++){
    float current_mA = ina219.getCurrent_mA();
//    float loadvoltage = ina219.getBusVoltage_V() + (ina219.getShuntVoltage_mV() / 1000);
    float loadvoltage = ina219.getBusVoltage_V();
    energy += loadvoltage * current_mA *interval/1000/ 3600;
    current_avg += current_mA;
    voltage_avg += loadvoltage;
    delay(interval);
    }
    current_avg /=samplingFreq;
    voltage_avg /=samplingFreq;
  }

 void timeConvert(unsigned long currentSecond){
  if (currentSecond < 3600){
    display.print("sec");
    display.setCursor(20, 4);
    display.print(round(currentSecond));
  } else if ((currentSecond >= 3600) && (currentSecond >= 3600L*12)) {
    display.print("min");
    display.setCursor(20, 4);
    display.print(currentSecond/60, 1);
  } else {
    display.print("hr");
    display.setCursor(20, 4);
    display.print(currentSecond/3600, 1);  
  } 
 }

