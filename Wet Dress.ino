#include <LoRa.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <Servo.h>
#include "Adafruit_MAX31855.h"

float TC [2]= {0,0};

//declare SPI pins for thermocouple breakout
int MAXDO = 50;
int TCCS [2] = {49,48};
int MAXCS;
int MAXCLK = 52;
int chipSelect = 53;

float PT [2] = {0,0};

//initialize thermocouple breakout board
Adafruit_MAX31855 thermocouple1(MAXCLK, TCCS[0], MAXDO);
Adafruit_MAX31855 thermocouple2(MAXCLK, TCCS[1], MAXDO);


Adafruit_MAX31855 Thermocouples [2] = {thermocouple1,thermocouple2}; //create thermocouple array

//define pins for pressure transducer data
byte Pins[3] = {A0,A1};

//create a variable for the critical parameter of the oxidizer
String CRIT;
String Data = "";

//define pins, variables for oxidizer venting control
int VENT = 20;
int ventBegin;
int ventTime;

////////////////////////////////////////////////


void setup() {
  //begin serial communication for debugging
  Serial.begin(9600);

  //begin communication with load cells
  Serial1.begin(9600);
  //Serial2.begin(9600);

  Serial.println("STABLIZING THERMOCOUPLES...");
  // wait for MAX chip to stabilize
  delay(1000);
  Serial.println("THERMOCOUPLES STABILIZED");

  //initialize pressure transducers
  pinMode(Pins,INPUT);
  Serial.println("SENSORS READY");
  CRIT = "STABLE";

  //initialize vent timer
  pinMode(VENT,INPUT);
  ventBegin = millis();
  
  ////Data storage initialization////
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
  }
  else{
    File dataFile = SD.open("Flight.txt", FILE_WRITE);
    if (dataFile){
        dataFile.println("Beginning New Engine Profile");
        dataFile.close();
        Serial.println("New Datalog Created");
      }
    else{
        Serial.println("error opening file Flight.txt");
        return;
      }
    Serial.println("Initialization Complete");
  }
}




////////////////////////////////////////////////




void loop() {

    Serial.print("Starting");
    CRIT = "STABLE";
    for(int j = 0; j < 2; j++){
      MAXCS = TCCS[j]; //change CS line depending on which thermocouple data is taken from
      TC[j] = Thermocouples[j].readCelsius(); //take temperature data in celsius
    }
    //Serial.println("Taking pressure...");
    for(int k = 0; k < 2; k++){
      PT[k] = ((analogRead(Pins[k]) - 100) / 820) * 5076; //take pressure data from transducers, adjust for factory calibration 
    }
    
    //Serial.println("Checking for critical temperature");
    for (int l = 0; l < 2; l++){ //test for critical temperature: open solenoid if above threshold, close if below
      if(TC[l] >= 309.5){
          CRIT = "TEMP";
          Serial.println("TEMPERATURE CRITICAL");
      }
    }

    //Serial.println("Checking for critical pressure");
    for (int m = 0; m < 2; m++){ //test for critical pressure: open solenoid if above threshold, close if below
      if(PT[m] >= 7240){
        CRIT = "PRESSURE";
        Serial.println("PRESSURE CRITICAL");
      }
    }
  
  ////Calculating venting time////

  if(digitalRead(VENT == LOW)){
    ventBegin = millis();
    ventTime = 0;
  }else{
    ventTime = millis()-ventBegin;
  }
  
  ////Data processing////

  String Data = "";
  for (int n = 0; n < 2; n++){
    Data+=TC[n];
    Data+=" ";
  }
  for (int x = 0; x < 2; x++){
    Data+=PT[x];
    Data+=" ";
  }
  Data+=CRIT;

  //append load cell data
  Data+=Serial1.read(); 
  
  //append vent timer data
  Data+=" ";
  Data+=ventTime;
  
  ////Data Storage////
  File dataFile = SD.open("Flight.txt", FILE_WRITE);

  if (dataFile){
    dataFile.println(Data);
    dataFile.close();
  }
  else{
    Serial.println("error opening file Flight.txt");
  }
  
  ////Data transmission////
  LoRa.beginPacket();
  LoRa.print(Data);
  LoRa.endPacket();
  Serial.println(Data);
}
