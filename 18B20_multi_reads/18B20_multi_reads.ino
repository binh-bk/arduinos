// forked directly from Mutiple/Dallas Temperature 
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 9
#define TEMPERATURE_PRECISION 10
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress one, two, three, four;

// Assign address manually. The addresses below will beed to be changed
// to valid device addresses on your bus. Device address can be retrieved
// by using either oneWire.search(deviceAddress) or individually via
// sensors.getAddress(deviceAddress, index)
// DeviceAddress insideThermometer = { 0x28, 0x1D, 0x39, 0x31, 0x2, 0x0, 0x0, 0xF0 };
// DeviceAddress outsideThermometer   = { 0x28, 0x3F, 0x1C, 0x31, 0x2, 0x0, 0x0, 0x2 };



void setup(void){
  Serial.begin(9600);
  sensors.begin();
  // locate devices on the bus
  Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: ");
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");

  // Search for devices on the bus and assign based on an index. Ideally,
  // you would do this to initially discover addresses on the bus and then
  // use those addresses and manually assign them (see above) once you know
  // the devices on your bus (and assuming they don't change).
  //
  // method 1: by index
  if (!sensors.getAddress(one, 0)) Serial.println("Unable to find address for Device 0");
  if (!sensors.getAddress(two, 1)) Serial.println("Unable to find address for Device 1");
  if (!sensors.getAddress(three,2)) Serial.println("Unable to find address for Device 2");
  if (!sensors.getAddress(four, 3)) Serial.println("Unable to find address for Device 3");



  // set the resolution to 8/9/10 bit per device
  sensors.setResolution(one, TEMPERATURE_PRECISION);
  sensors.setResolution(two, TEMPERATURE_PRECISION);
  sensors.setResolution(three, TEMPERATURE_PRECISION);
  sensors.setResolution(four, TEMPERATURE_PRECISION);

    // show the addresses we found on the bus
  Serial.print("0 Ad: ");
  printAddress(one);
  Serial.print("\tResolution: ");
  Serial.println(sensors.getResolution(one), DEC);
  Serial.print("1 Ad: ");
  printAddress(two);
  Serial.print("\tResolution: ");
  Serial.println(sensors.getResolution(two), DEC);
  Serial.print("2 Ad: ");
  printAddress(three);
  Serial.print("\tResolution: ");
  Serial.println(sensors.getResolution(three), DEC);
  Serial.print("3 Ad: ");
  printAddress(four);
  Serial.print("\tResolution: ");
  Serial.println(sensors.getResolution(four), DEC);


  Serial.println("Starting logging");
  Serial.println("Temperature by Address");
  
  printAddress(one);
  Serial.print("\t");
  printAddress(two);
  Serial.print("\t");
  printAddress(three);
  Serial.print("\t");
  printAddress(four);
  Serial.print("\n");
}


/*
   Main function, calls the temperatures in a loop.
*/
void loop(void)
{
  // call sensors.requestTemperatures() to issue a global temperature
  // request to all devices on the bus
  sensors.requestTemperatures();
  printData(one);
  Serial.print("\t");
  printData(two);
  Serial.print("\t");
  printData(three);
  Serial.print("\t");
  printData(four);
  Serial.print("\n");
  delay(2000);
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress){
  for (uint8_t i = 6; i < 8; i++){
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}
// main function to print information about a device
void printData(DeviceAddress deviceAddress){
  float tempC = sensors.getTempC(deviceAddress);
//  Serial.println("");
//  printAddress(deviceAddress);
  Serial.print(tempC);
//  return tempC;
}
