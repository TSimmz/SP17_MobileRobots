#include <math.h>
#include <Wire.h>
#include <Servo.h>
#include <Adafruit_RGBLCDShield.h>

Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

#define RED 0x1
#define GREEN 0x2
#define BLUE 0x4
 
#define S0 4
#define S1 5
#define S2 6
#define S3 7
#define sensorOut 8

const int SFSensor = A0;  //Short front sensor
const int SLSensor = A1;  //Short left sensor  
const int SRSensor = A2;  //Short right sensor

Servo LServo;  // Left servo
Servo RServo;  // Right servo

int options = 0;
int disp;
int b = 0;
int fRed = 0, fGreen = 0, fBlue = 0;
float avg;
int flag;
char DetColor[]={'N'};

#define kP 0.19
#define goal 180
#define Vel 1500
#define LMAX 1570
#define RMAX 1430

void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(9600);

  lcd.begin(16, 2);
  lcd.print("F:");
  lcd.setCursor(0,1);
  lcd.print("R:");
  lcd.setCursor(7,0);
  lcd.print("L:");
  lcd.setCursor(7,1);
  lcd.print("C:");
  
  // Servo set up
  LServo.attach(2);
  RServo.attach(3);

  LServo.write(Vel);
  RServo.write(Vel);
}

void loop() {
  uint8_t buttons = lcd.readButtons();

  // Setting red filtered photodiodes to be read
  digitalWrite(S2,LOW);
  digitalWrite(S3,LOW);
  // Reading the output frequency
  fRed = pulseIn(sensorOut, LOW);
  delay(75);

  // Setting Green filtered photodiodes to be read
  digitalWrite(S2,HIGH);
  digitalWrite(S3,HIGH);
  fGreen = pulseIn(sensorOut, LOW);
  delay(75);

  // Setting Blue filtered photodiodes to be read
  digitalWrite(S2,LOW);
  digitalWrite(S3,HIGH);
  fBlue = pulseIn(sensorOut, LOW);
  delay(75);
  
  colorDisplay(fRed, fGreen, fBlue);
 
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

    disp = 2;
    float sl_val = readSensor(SLSensor);
    sensorDisplay(sl_val);
      
    speedSensor(sr_val, sf_val, sl_val);
  }
}

void speedSensor(float right, float front, float left)
{ 
  avg = (right + left) / 2;
  
  float uRT = kP * (avg - right);
  float uFT = kP * (goal - front);

  if (uFT > 0){
    if (uRT < 0){
      LServo.write(LMAX + uRT);
      RServo.write(RMAX); 
    } 
    else{
      LServo.write(LMAX);
      RServo.write(RMAX + uRT);
    }
  }
  else if (uFT < 50){
    LServo.write(1540);
    RServo.write(1540);
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
  setCurs(disp);

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

void setCurs(int d){
  switch(d){
    case 0:
      lcd.setCursor(3,1); break;
    case 1:
      lcd.setCursor(3,0); break;
    case 2:
      lcd.setCursor(9,0); break;
  }
}

//---------------------------------------------------------
// colorDisplay- takes in color readings and displays
//---------------------------------------------------------
void colorDisplay(int red, int green, int blue)
{
  //Serial.println(red);
  //Serial.println(green);
  //Serial.println(blue);
  
  lcd.setCursor(9,1);
  
  //---GREEN CHECK--------------------------------------------------------------------
  if ((red > 35 && red < 41) && (green > 35 && green < 41) && (blue > 35 && blue < 41))
  {
    lcd.setBacklight(GREEN);
    lcd.print("F");
  }
  //---RED CHECK---------------------------------------------------------------------
  else if ((red > 10 && red < 16) && (green > 13))
  {
    lcd.setBacklight(RED);
    lcd.print("R"); 
    b = 1;  
  }
  //---BLUE CHECK----------------------------------------------------------------------
  else if (blue < 13)
  {
    lcd.setBacklight(BLUE);
    lcd.print("B");
    if (b == 1){
       LServo.detach();
       RServo.detach();
    }
  }  
}
