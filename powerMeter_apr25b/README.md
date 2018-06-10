Arduino code using ProMini(328P), current sensor (INA219), a 0.96" (128x64) OLED, a SD module, a boost-up 3.7V from Li-Po to 5V
=======
 * original post by GreatScotLab is here https://www.instructables.com/id/Make-Your-Own-Power-MeterLogger/
 * more about ina219 current sensor: https://learn.adafruit.com/adafruit-ina219-current-sensor-breakout/wiring
 * changes by Binh Nguyen, April 26, 2018
 * - use EEPROM to store ONE BYTE to icnrease one value, and write to header in setup loop
 * - use one log file (data.txt) instead of three
 * - add a message for logfile status in setup loop
 * - enlarge to current display (minor)
 * - add the time inverval in energy calculation (adding 100 ms)
 * + still using SSD_1306_32 in 128x64 OLED, remove comment line 73 to activate 128x64 mode. 
 * + memory on Arduino ProMini (5V) 328P is only enough for 128x64 mode.
 * USB boost up (Vout = 5V, experiment with a load of 100mA) need to have 3.3 V input.
 * even with Vin = 4.2V, the USB Boost up cannot generate 5V at 600 mA as advertised (the Vout reduces to 4.83V) //June 2018

