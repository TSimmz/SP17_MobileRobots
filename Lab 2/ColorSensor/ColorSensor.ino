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

int fRed = 0;
int fGreen = 0;
int fBlue = 0;
char DetColor[]={'N'};
int flag1, flag2 = 0;

void setup() {
  lcd.begin(16,2);
  lcd.setCursor(1,6);
  
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(sensorOut, INPUT);
  
  // Setting frequency-scaling to 20%
  digitalWrite(S0,HIGH);
  digitalWrite(S1,LOW);
  
  Serial.begin(9600);
}


void loop() {
  // Setting red filtered photodiodes to be read
  digitalWrite(S2,LOW);
  digitalWrite(S3,LOW);
  // Reading the output frequency
  fRed = pulseIn(sensorOut, LOW);

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
  Serial.println();
}

void colorDisplay(int red, int green, int blue)
{
  if(flag1 != flag2){
    flag2 = flag1;
    lcd.clear();
  }
  lcd.setCursor(1,6); 
    
  if ((red > 30 && red < 60) && (green > 35 && green < 75) && (blue > 28 && blue < 40))
  {
    lcd.setBacklight(BLUE);
    lcd.print("BLUE");
    flag1 = 0;
    
  }
  else if ((red > 10 && red < 30) && (green > 55 && green < 80) && (blue > 30 && blue < 75))
  {
    lcd.setBacklight(RED);
    lcd.print("RED");
    flag1 = 1;
  }
  else if ((red > 150 && red < 215) && (green > 170 && green < 225) && (blue > 130 && blue < 180))
  {
    lcd.setBacklight(GREEN);
    lcd.print("FLOOR");
    flag1 = 2;
  }
}

