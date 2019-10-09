#include <BMx280I2C.h>
#include <BMx280MI.h>
#include <BMx280SPI.h>
#include <BMx280SPIClass.h>
#include <BMx280TwoWire.h>


#include "FrequencyTimer2.h"

#define DISPLAY_COLS  240   // 表示用のバッファサイズ 最大256-16
#define LED_COMMON   1    // LEDの極性。カソードがROW側(Aタイプ)は0、COL側(Bタイプ)は1

const static
//       /*LED  */ -- 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16
byte pins[17]= /*Arduino*/ {-1, 5, 4, 3, 2,14,15,16,17,13,12,11,10, 9, 8, 7, 6};

const static
byte cols[8] = {pins[13],pins[ 3],pins[ 4],pins[10],pins[ 6],pins[11],pins[15],pins[16]};
const static
byte rows[8] = {pins[ 9],pins[14],pins[ 8],pins[12],pins[ 1],pins[ 7],pins[ 2],pins[ 5]};

byte col = 0;
byte leds[DISPLAY_COLS];
byte pattern=0;
byte pattern_max=0;

#define I2C_ADDRESS 0x76
//create a BMx280I2C object using the I2C interface with I2C Address 0x76
BMx280I2C bmx280(I2C_ADDRESS);

void setup() {
  byte i;
  for(i=1;i<=16;i++) pinMode(pins[i], OUTPUT);      // sets the pins as output
  for(i=1;i<= 8;i++) digitalWrite(cols[i - 1], LOW);   // set up cols and rows
  for(i=1;i<= 8;i++) digitalWrite(rows[i - 1], LOW);
  clearLeds();
  FrequencyTimer2::disable();               // Turn off toggling of pin 11
  FrequencyTimer2::setPeriod(2000);            // Set refresh rate
  FrequencyTimer2::setOnOverflow(display);        // Set interrupt routine to be called

  
    // put your setup code here, to run once:
  Serial.begin(9600);

  //wait for serial connection to open (only necessary on some boards)
  while (!Serial);

  Wire.begin();

  //begin() checks the Interface, reads the sensor ID (to differentiate between BMP280 and BME280)
  //and reads compensation parameters.
  if (!bmx280.begin())
  {
    Serial.println("begin() failed. check your BMx280 Interface and I2C Address.");
    while (1);
  }

  if (bmx280.isBME280())
    Serial.println("sensor is a BME280");
  else
    Serial.println("sensor is a BMP280");

  //reset sensor to default parameters.
  bmx280.resetToDefaults();

  //by default sensing is disabled and must be enabled by setting a non-zero
  //oversampling setting.
  //set an oversampling setting for pressure and temperature measurements. 
  bmx280.writeOversamplingPressure(BMx280MI::OSRS_P_x16);
  bmx280.writeOversamplingTemperature(BMx280MI::OSRS_T_x16);

  //if sensor is a BME280, set an oversampling setting for humidity measurements.
  if (bmx280.isBME280())
    bmx280.writeOversamplingHumidity(BMx280MI::OSRS_H_x16);
    
  setPattern("Hello! I'm Arduino.");
}

void loop() {
  pattern = ++pattern;
  if(pattern > pattern_max){
    String buf="";
    pattern=0;

    //start a measurement
    if (!bmx280.measure())
    {
      Serial.println("could not start measurement, is a measurement already running?");
      return;
    }
  
    //wait for the measurement to finish
    do
    {
      delay(100);
    } while (!bmx280.hasValue());
  
    Serial.print("Pressure: "); Serial.println(bmx280.getPressure());
    buf+=String(bmx280.getPressure())+"hPa ";
    Serial.print("Temperature: "); Serial.println(bmx280.getTemperature());
    buf+=String(bmx280.getTemperature())+"c ";
  
    if (bmx280.isBME280())
    {
      Serial.print("Humidity: "); 
      Serial.println(bmx280.getHumidity());
      buf+=String(bmx280.getHumidity())+"%";
    }
    int len=buf.length();
    char buf_s[len];
    buf.toCharArray(buf_s,len+1);
    clearLeds();
    setPattern(buf_s);
  }
  
  delay(75);
}

void clearLeds() {
  for(int i = 0; i < DISPLAY_COLS; i++) leds[i] = 0x00;  // Clear display array
}

void setPattern(char *s) {
  int i=0,j;
  pattern=0;
  while(s[i] != '\0' && pattern < DISPLAY_COLS-8){
    pattern += Font_Draw(s[i],pattern,-1);
    i++;
  }
  pattern_max = pattern+16;
  pattern=0;
}

void display() {
  byte row;
  digitalWrite(cols[col], LED_COMMON);          // Turn whole previous column off
  col++;
  if (col == 8) col = 0;
  
  for (row = 0; row < 8; row++) {
    if( col+pattern >= 8 &&
      col+pattern < pattern_max-8 &&
      (leds[col+pattern-8]>>row)&1 == 1  ){
      digitalWrite(rows[row], LED_COMMON);      // Turn on this led
    } else digitalWrite(rows[row], !LED_COMMON);
  }
  digitalWrite(cols[col], !LED_COMMON);
}
