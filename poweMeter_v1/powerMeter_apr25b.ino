/*
 * original post by GreatScotLab is here https://www.instructables.com/id/Make-Your-Own-Power-MeterLogger/
 * more about ina219 current sensor: https://learn.adafruit.com/adafruit-ina219-current-sensor-breakout/wiring
 * changes by Binh Nguyen, April 26, 2018
 * - use EEPROM to store ONE BYTE to icnrease one value, and write to header in setup loop
 * - use one log file (data.txt) instead of three
 * - add a message for logfile status in setup loop
 * - enlarge to current display (minor)
 * - add the time inverval in energy calculation (adding 100 ms)
 * + stil using SSD_1306_32 in 128x64 OLED, remove comment line 73 to activate 128x64 mode. 
 * + memory on Arduino ProMini 328P is only enough for 128x32 mode.

*/
#include<EEPROM.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include "SdFat.h"
SdFat SD;

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);
Adafruit_INA219 ina219;

#define samplingFreq  10
int interval = 0;
unsigned long previousSecond = 0;
float current_mA = 0;
float loadvoltage = 0;
float energy = 0;
byte id;
String fileName = "";
File logFile;

void setup() {
  SD.begin(10);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  ina219.begin();
  
  id = EEPROM.read(1) + 1;
  fileName = "f" + String(id)+ ".txt";
  String header = "time, V, ma, mWh\ntest"+String(id);
  EEPROM.write(1, id);
  
  id = EEPROM.read(0) + 1;
  String header = "time, mA, V, mWh\ntest"+String(id);
  EEPROM.write(0, id);
  
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.print(fileName);
  display.setCursor(10, 10);

  interval = 1000/samplingFreq;
  logFile = SD.open(fileName, FILE_WRITE);
  if (logFile) {
      logFile.println(header);
      logFile.close();
      display.print("OK");
    } else display.print("failed");
  display.display();
  delay(1000);
}

void loop() {
  unsigned long currentSecond = millis()/1000;
  displaydata();
  ina219reads();
  if (currentSecond - previousSecond >= 10)  {
    previousSecond = currentSecond;
    String buff  = String(currentSecond) + "," + String(loadvoltage) + "," \
    + String(current_mA) + "," + String(energy);
    
    logFile = SD.open("data.txt", FILE_WRITE);
    if (logFile) {
      logFile.println(buff);
      logFile.close();
    }
  }
//   delay(100);
}

void displaydata() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(50, 0);
  display.print(round(current_avg));
  display.setTextSize(1);
  display.setCursor(105, 0);
  display.print("mA");
  display.setCursor(0, 0);
  display.print(voltage_avg);
  display.setCursor(30, 0);
  display.print("V");
  display.setCursor(0, 10);
  display.print("f:");
  display.print(id);
  display.setCursor(0, 20);
  display.print(round(voltage_avg * current_avg));
  display.setCursor(35, 20);
  display.print("mW");
  display.setCursor(65, 20);
  display.print(round(energy));
  display.setCursor(100, 20);
  display.print("mWh");
  display.display();
}
 void ina219reads(){
  for (int i=0; i<samplingFreq; i++){
    float current_mA = ina219.getCurrent_mA();
    float loadvoltage = ina219.getBusVoltage_V() + (ina219.getShuntVoltage_mV() / 1000);
//    float loadvoltage = ina219.getBusVoltage_V();
    energy += loadvoltage * current_mA *1/ 3600;
    current_avg += current_mA;
    voltage_avg += loadvoltage;

    delay(interval);
    }
    current_avg /=samplingFreq;
    voltage_avg /=samplingFreq;
  }





