#include <math.h>
#include <Wire.h>
#include <Servo.h>
#include <Adafruit_RGBLCDShield.h>

Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

const int SFSensor = A0;  //Short front sensor
const int SLSensor = A1;  //Short left sensor  
const int SRSensor = A2;  //Short rRead sensor

Servo LServo;  // Left servo
Servo RServo;  // rRead servo

int rRead, fRead;
int options = 0;
int disp;

#define kP 0.5
#define goal 200
#define mins 135
#define Vel 1500
#define LMAX 1570
#define RMAX 1430

void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(9600);

  lcd.begin(16, 2);
  lcd.print("Front:");
  lcd.setCursor(0,1);
  lcd.print("Right:");

  // Servo set up
  LServo.attach(2);
  RServo.attach(3);

  LServo.write(Vel);
  RServo.write(Vel);
}

void loop() {
  uint8_t buttons = lcd.readButtons();
 
  if (buttons) {
    if (buttons & BUTTON_UP) {
      options = 1;
    } 
  }

  if (options == 1){

  
    disp = 0;
    float sr_val = readSensor(SRSensor);
    sensorDisplay(sr_val);
   
    disp = 1;
    float sf_val = readSensor(SFSensor);
    sensorDisplay(sf_val);
      
    speedSensor(sr_val, sf_val);
  }
}

void speedSensor(float right, float front)
{ 
  float uRT = kP * (goal - right);
  float uFT = kP * (goal - front);
  
  lcd.setCursor(12,1);
  lcd.print(uRT);
  lcd.setCursor(12,0);
  lcd.print(uFT);

  if (uFT > 0){
    if (uRT < 0){
      LServo.write(LMAX + uRT/2);
      RServo.write(RMAX); 
    } 
    else{
      LServo.write(LMAX);
      RServo.write(RMAX + uRT/2);
    }
  }
  else if(uFT < 0){
    LServo.write(1360);
    RServo.write(1360);
  }
}

//------------------------------------------------
// readSensor - provided by FrontRawSensor sketch
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
  setCurs();

  if (val < 90){
    lcd.print(">10\"");
  }else if (val <= 108 && val >= 98){
    lcd.print(" 10\"");
  }else if (val <= 128 && val > 105){
    lcd.print("  9\"");
  }else if (val <= 140 && val > 125){
    lcd.print("  8\"");
  }else if (val <= 163 && val > 135){
    lcd.print("  7\"");
  }else if (val <= 187 && val >= 160){
    lcd.print("  6\"");
  }else if (val <= 218 && val >= 192){
    lcd.print("  5\"");
  }else if (val <= 261 && val >= 227){
    lcd.print("  4\"");
  }else if (val <= 335 && val >= 290){
    lcd.print("  3\"");
  }else if (val <= 453 && val >= 404){
    lcd.print("  2\"");
  }else if (val <= 644 && val >= 609){
    lcd.print("  1\"");
  }else if ( val > 600){
    lcd.print(" <1\"");
  }
}

void setCurs(){
  if(disp == 0){
    lcd.setCursor(6,1);
  }
  else{
    lcd.setCursor(6,0);
  }
}
