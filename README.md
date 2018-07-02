# Arduino codes, schematics of the following projects:
### 1. powerMeter_v1:  
 portable power meter using INA219 current sensor, powered by Lithium batter and loging data to SD card (Credit to GreatScott Lab)
### 2. powerMeter_v2:  
power meter using INA219 and 18B20 temperature probe, Nokia 5110 screen, and log data to a computer via Serial communication by a python script 
### 3. powerMeter_v2.1:
tools for LED evaluation: based v2 power meter, added TLS2561 luminosity sensor and photocell, log data via Serial by python script

### 4. simpleControlTerminal:
a simple NTP clock with a terminal to publish analog reading to MQTT server, can publish to 4 seperated channels using WeMOS Mini-Lite ESP8266, SSD1306 OLED. A more details on making a subscriber-publisher with MQTT server http://www.instructables.com/id/From-Flashlight-to-Motion-Sensor-With-ESP8266-and-/

### 5. dimmerPir:
control 4x3W LED by motiion sensor or  with MQTT with Json format, intensity is parsed out by the controller, or it can be turned on by a switch, using ESP8266 -01. The current is limited by a MOSFET, a transitor and 1ohm (3W), allows about 600 mA through LEDS as the max output. LEDs is automatically turned on with PIR sensor or with publish message from MQTT server

### 6. 8 LEDs Dimmer
Boost 5V input to 26V using MT3608 and turn on 10LEDs (0.5W each, 8mm superbright). The brightness is controlled by 8266 esp01 using JSON message from MQTT server, or a manual push button. The current runs through the LEDs is limited by MOSFET and a transitor with 4ohm (about 150mA max).

# Schematics

### 1. powerMeter_v1
<p align="center">
  <img src="https://github.com/binh-bk/arduinos/blob/master/poweMeter_v1/powerMeter_1.jpg"/>
</p>

### 2. powerMeter_v2:  
<p align="center">
  <img src="https://github.com/binh-bk/arduinos/blob/master/powerMeter_v2/powerMeter_v2.jpg"/>
</p>

### 3. powerMeter_v2.1:
<p align="center">
  <img src="https://github.com/binh-bk/arduinos/blob/master/powerMeter_v2.1/powerMeter_v2.1a.jpg"/>
</p>

### 4. Simple Terminal with NTP Clock and Publish Control Value
<p align="center">
<img src="https://github.com/binh-bk/arduinos/blob/master/simpleControlTerminal/simpleTerminal.jpg"/>
</p>

### 5. PIR and LED dimmer
<p align="center">
<img src="https://github.com/binh-bk/arduinos/blob/master/dimmerPir/pir_LED.jpg"/>
</p>

### 6. 10 LEDs and 5V supply
<p align="center">
<img src="https://github.com/binh-bk/arduinos/blob/master/ledDimmer_MQTT/flashlight_ESP8266.jpg"/>
</p>

### to be added...
