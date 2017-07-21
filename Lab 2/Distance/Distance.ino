//*************************************
// Distance
// Tyler Simoni
// Esthevan Romeiro
//*************************************

#include <math.h>
#include <Wire.h>
#include <Servo.h>
#include <Adafruit_RGBLCDShield.h>

const int SFSensor = A0;  //Short front sensor

Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

// Defining the colors
#define RED 0x1
#define YELLOW 0x3
#define GREEN 0x2
#define TEAL 0x6
#define BLUE 0x4
#define VIOLET 0x5
#define WHITE 0x7

Servo LServo;
Servo RServo;

int options = 0;
int distRead = -1;
int goal = 200;
float kP = .5;
#define Vel 1500

void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(9600);

  // Servo set up
  LServo.attach(2);
  RServo.attach(3);

  LServo.write(Vel);
  RServo.write(Vel);
  
  // Starts LCD, sets to white and prints initial message
  lcd.begin(16, 2);
  lcd.setBacklight(WHITE);
  lcd.print("TASK 1: Distance");
}

void loop() {
  uint8_t buttons = lcd.readButtons();
 
  if (buttons) {
    lcd.clear();
    lcd.setCursor(0,0);

    // Front sensor
    if (buttons & BUTTON_UP) {
      lcd.print("FRONT SENSOR");
      lcd.setBacklight(RED);
      options = 1;
    } 
  }
  //******************************************
  // Each option reads the sensor and displays
  // using the sensorDisplay function
  //******************************************
  if (options == 1)
  {
    float sf_val = readSensor(SFSensor);
    sensorDisplay(sf_val);
    speedSensor(sf_val);
  }
}
void speedSensor(float front)
{ 
  float uRT = kP * (goal - front);
 
  if (uRT <= 0){
    LServo.detach();
    RServo.detach();
    lcd.clear();
    lcd.print("______GOAL______");
  }
  else
  {
    LServo.write(Vel + uRT);
    RServo.write(Vel - uRT);
  }
}
//------------------------------------------------
// readSensor
//------------------------------------------------
int readSensor(int sensor){
  float sum = 0, avg = 0;
  
  for (int i = 1; i <= 10; i++) {
   sum = sum + analogRead(sensor);
  }
  delay(25);
  return (sum / 10);
}

//*********************************************
// Chooses the distance to display based on the
// sensor reading passed into the function.
//*********************************************
void sensorDisplay(float val)
{
  lcd.setCursor(5,1);
  
  if (val < 90){
    lcd.print(">10\"");
  }else if (val <= 108 && val >= 98){
    lcd.print("_10\"");
  }else if (val <= 128 && val > 105){
    lcd.print("__9\"");
  }else if (val <= 140 && val > 125){
    lcd.print("__8\"");
  }else if (val <= 163 && val > 135){
    lcd.print("__7\"");
  }else if (val <= 187 && val >= 160){
    lcd.print("__6\"");
  }else if (val <= 218 && val >= 192){
    lcd.print("__5\"");
  }else if (val <= 261 && val >= 227){
    lcd.print("__4\"");
  }else if (val <= 335 && val >= 290){
    lcd.print("__3\"");
  }else if (val <= 453 && val >= 404){
    lcd.print("__2\"");
  }else if (val <= 644 && val >= 609){
    lcd.print("__1\"");
  }else if ( val > 600){
    lcd.print("_<1\"");
  }
}
