//Library codes
#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>
#include <Servo.h>
#include <stdlib.h>

#include "Maze.h"
#include "Position.h"
#include "StackArray.h"

// RGB SHIELD
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

//Defining Color Sensors
#define S0 4
#define S1 5
#define S2 6
#define S3 7
#define sensorOut 8

//Defining Colors for LCD
#define RED 0x1
#define YELLOW 0x3
#define GREEN 0x2
#define TEAL 0x6
#define BLUE 0x4
#define VIOLET 0x5
#define WHITE 0x7

//Servo Setup
Servo LServo;
Servo RServo;

//Distance Sensors
const int SFSensor = A0; //Short range front sensor
const int SLSensor = A1; //Short range left sensor
const int SRSensor = A2; //Short range right sensor
const int LFSensor = A3; //Long range front sensor

//Encoders
int lEncPin = 10; //Left wheel encoder
int rEncPin = 11; //Right wheel encoder
int rEncCount; //Counter for right encoder
int lEncCount; //Counter for left encoder
int lEncLast = 0; //Last number recorded by left encoder?
int rEncLast = 0; //Last number recorded by right encoder?

//Some global definitions
#define writeMicroseconds write
#define STOP 1500

uint8_t buttons;          //Declaration of buttons
char grid[16] = {'O'};    // Grid Array
int pos[5] = {0};         // U: -1, O: 0, W: 1
int options = 0;          // Options for maze choice
bool startFlag = false;   // Flag for starting movement

int KP = 8;
int error = 0;

direction compass;
Position currPos;
StackArray<direction> Heading;
StackArray<Position> Path;

//Temporary global variables
int startingPos, endingPos; //holds the starting and ending grid position

// Holds the returned distance value
int f, l, r;              
int FrontSensorValue, RightSensorValue, LeftSensorValue;

Maze* maze;

/* 
   *  Pos[] elements:
   *  0 = Grid Location 0-15
   *  1 = SOUTH
   *  2 = WEST
   *  3 = NORTH
   *  4 = EAST
*/

//**************************************************************
// Initial setup
//**************************************************************
void setup() 
{
  // Setting up the servos
  LServo.attach(2); 
  RServo.attach(3);

  // Set initial speed to 0;
  RServo.write(STOP);
  LServo.write(STOP);

  // Setting up RGB shield
  Serial.begin(9600);

  // Initializing encoder counts;
  RefreshCount();

  // Setting up the LCD screen
  lcd.begin(16, 2);

  //Setting up a friendly message
  lcd.setBacklight(WHITE);
  lcd.setCursor(0,0);
  lcd.print("Robots-Lb3-T1");
  

  // Setting the encoders
  pinMode(lEncPin, INPUT_PULLUP);
  pinMode(rEncPin, INPUT_PULLUP);

  // Setting color sensors
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(sensorOut, INPUT);
  
  // Setting up frequency scaling to 20%
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);

  maze = new Maze();     //Creates the borders of the maze
  compass = EAST;        //Sets the inital heading
  currPos = maze->start; //Sets currPos to the start
  Path.push(currPos);    //Pushes current position onto stack
  Heading.push(compass); //Pushes current heading onto stack
  pos[0] = 1;            //Sets initial grid location to 1
   
  MazeSelect();          //Select which maze to run
}

//**************************************************************
// Main execution loop
//**************************************************************
void loop() 
{
  PositionScan();     // Sets values in position array
  PositionDisplay();  // Displays heading on LCD
  
  GridDisplay();      // Displays array of grid positions

  ChooseDirection();
  
  if(isComplete())
  {
    CompleteMessage();
  }
}

//**************************************************************
// Allows user to select the maze to initialize
//**************************************************************
void MazeSelect()
{
  while(!startFlag)
  {
    buttons = lcd.readButtons();
    if (buttons) 
    {
      lcd.clear();
      lcd.setCursor(0,0);
      
      if (buttons & BUTTON_LEFT) 
      {
        lcd.print("1 - Maze 1");
        options = 1;
      }
  
      else if (buttons & BUTTON_UP)
      {
        lcd.print("2 - Maze 2");
        options = 2;
      }
      else if (buttons & BUTTON_RIGHT)
      {
        lcd.print("3 - Maze 3");
        options = 3;
      }
      else if (buttons & BUTTON_SELECT)
      {
        switch (options)
        {
          case 1:
            maze->init_Maze1();    // Maze 1 initialization 
            startFlag = true;
            break;
          case 2:
            maze->init_Maze2();    // Maze 2 initialization 
            startFlag = true; 
            break;
          case 3:
            maze->init_Maze3();    // Maze 3 initialization  
            startFlag = true;
            break;
          default:
            maze->init_Maze1();    // Default Maze 1 
            startFlag = true; 
            break;
        }
      }
    }
  }
}

//**************************************************************
// Chooses and moves next direction based on maze
//**************************************************************
void ChooseDirection()
{
  int i = 0, n = 0, t = 0;
  direction d, e;
  Position nbr1, nbr2;
  
  if (maze->getState(currPos) != VISITED)
    maze->setState(currPos, VISITED);       // Sets our current location as visited
  
 
  for(d = compass; i < 4; d = next_dirR(d), i++)
  {
    nbr1 = currPos.Neighbor(d, 1);          //Checks immediate neighbor
    if (maze->getState(nbr1) == WALL)
      n++;
    else{
      nbr2 = currPos.Neighbor(d, 2);        //Checks neighbor's neighbor
      if (maze->getState(nbr2) == OPEN){
        TurnCW(n);                          // Turns 90 degrees, n times;
        currPos = nbr2;                     // Updates position
        MoveForward();                      // Moves forward     
        return;
      }
      else
        n++;
    }
  }
  
  if(i == 4){    //If above loop did not find a space
    e = opposite_dir(Heading.peek());
    while(e != d)
    {
      d = next_dirR(d);
      t++;
    }
    Heading.pop();
    Path.pop();
    
    TurnCW(t);
    MoveForward();
    Path.pop();
    Heading.pop();
    
    currPos = Path.peek();
    return;
  }
}

//**************************************************************
// Moves forward 1 grid space
//  Set minimum to compare in order to fix deviation
//**************************************************************
void MoveForward()
{
  RefreshCount();
  
  int lSpeed = STOP + 70;
  int rSpeed = STOP - 63;
  
  int totalTicks = 0;
  
  ColorFlash(compass);
  
  while(lEncCount < 140 && rEncCount < 140)
  {
    LServo.write(lSpeed); 
    RServo.write(rSpeed); 

    EncoderRead();

    //rSpeed += error / KP;
    //totalTicks += lEncCount;
    
    //RefreshCount();
  }
  pos[0] = GetGrid(pos[0]);

  Path.push(currPos);
  Heading.push(compass);
  
  Stop();
}

//**************************************************************
// Turns clockwise 90 degrees for every value of turn
//**************************************************************
void TurnCW(int turns)
{
  if (turns == 0)
    return;
  
  for(int i = 0; i < turns; i++)
  {
    RefreshCount();
    while(lEncCount < 28 && rEncCount < 28)
    {
      LServo.write(STOP+70);
      RServo.write(STOP+70);
      EncoderRead();
    }
    compass = next_dirR(compass);
    Stop();
    delay(500); 
  }
}

//**************************************************************
// Stops the robot
//**************************************************************
void Stop()
{
  LServo.write(STOP);
  RServo.write(STOP);
}

//**************************************************************
// Reads distance sensor and returns 1 for Wall & 0 for Open
//**************************************************************
int WallScan(float sensor)
{  
  return (sensor <= 10) ? 1 : 0;
}

//**************************************************************
// Given compass direction, populates the position array with
// Wall or Open when called
//**************************************************************
void PositionScan()
{
  /* Pos[] elements:
   *  0 = Grid Location 0-15
   *  1 = SOUTH
   *  2 = WEST
   *  3 = NORTH
   *  4 = EAST
   */
  switch(compass)
  {
    case EAST:
      pos[1] = WallScan(RightSensor()); //SOUTH Wall/Open
      pos[2] = -1;                      //WEST always unknown
      pos[3] = WallScan(LeftSensor());  //NORTH Wall/Open
      pos[4] = WallScan(FrontSensor()); //EAST Wall/Open
      break;
    case WEST: 
      pos[1] = WallScan(LeftSensor());  //SOUTH Wall/Open
      pos[2] = WallScan(FrontSensor()); //WEST Wall/Open
      pos[3] = WallScan(RightSensor()); //NORTH Wall/Open
      pos[4] = -1;                      //EAST always unknown
      break;
    case SOUTH:  
      pos[1] = WallScan(FrontSensor()); //SOUTH Wall/Open
      pos[2] = WallScan(LeftSensor());  //WEST Wall/Open
      pos[3] = -1;                      //NORTH always unknown
      pos[4] = WallScan(RightSensor()); //EAST Wall/Open
      break;
    case NORTH:
      pos[1] = -1;                      //SOUTH always unknown
      pos[2] = WallScan(RightSensor()); //WEST Wall/Open
      pos[3] = WallScan(FrontSensor()); //NORTH Wall/Open
      pos[4] = WallScan(LeftSensor());  //EAST Wall/Open
      break;
  }
  delay(500); 
}

//**************************************************************
// Sets screen locations for Grid
//**************************************************************
void GridDisplay()
{
  //Replace O's in grid array with X's
  // as new grids are visited
  grid[pos[0]-1] = 'X';

  for(int i = 0; i < 16; i++)
  {
    lcd.setCursor(i,0);
    if (grid[i] == 'X')
      lcd.print('X');
    else
      lcd.print('O');
  }
}

//**************************************************************
// Sets screen locations for Locations
//**************************************************************
void PositionDisplay()
{
  //Format: G1 NX SX E0 WU
  lcd.setCursor(0,1); //Column 0, Row 1
  lcd.print("G");
  lcd.setCursor(1,1); //Column 1, Row 1
  lcd.print("  ");
  lcd.setCursor(1,1);
  lcd.print(pos[0]);
  lcd.setCursor(3,1); //Column 3, Row 1
  lcd.print("N");
  lcd.setCursor(6,1); //Column 6, Row 1
  lcd.print("S");
  lcd.setCursor(9,1); //Column 9, Row 1
  lcd.print("E");
  lcd.setCursor(12,1); //Column 12, Row 1
  lcd.print("W");
  
  //North Logic
  lcd.setCursor(4,1);
  switch(pos[3])
  {
    case -1:
      lcd.print("U");
      break;
    case 0:
      lcd.print("O");
      break;
    case 1:
      lcd.print("X"); 
  }

  //South Logic
  lcd.setCursor(7,1);
  switch(pos[1])
  {
    case -1:
      lcd.print("U");
      break;
    case 0:
      lcd.print("O");
      break;
    case 1:
      lcd.print("X"); 
  }

    //East Logic
  lcd.setCursor(10,1);
  switch(pos[4])
  {
    case -1:
      lcd.print("U");
      break;
    case 0:
      lcd.print("O");
      break;
    case 1:
      lcd.print("X"); 
  }

  //West Logic
  lcd.setCursor(13,1);
  switch(pos[2])
  {
    case -1:
      lcd.print("U");
      break;
    case 0:
      lcd.print("O");
      break;
    case 1:
      lcd.print("X"); 
  }
}

//**************************************************************
// Determines if the maze has been fully traversed
//**************************************************************
bool isComplete()
{
  //May change to scan visited nodes instead
  
  int count = 0;
  for(int i = 0; i < 16; i++)
  {
    if(grid[i] == 'X')
      count++;
    if(count == 15)
      return true;
  }
  return false;
}

//**************************************************************
// Determines if the maze has been fully traversed
//**************************************************************
int GetGrid(int index)
{
  //CALL THIS FUNCTION WHEN CROSSING BOUNDRIES MOVING FORWARD
  if(currPos.getCol() % 2 != 0 || currPos.getRow() % 2 != 0)
  {
    switch (compass)
    {
      case EAST:
        index += 1;
        break;
      case WEST:
        index -= 1;
        break;
      case NORTH:
        index -= 4;
        break;
      case SOUTH:
        index += 4;
        break;
      default:
        break;
    }
  }

  return index;
}

//**************************************************************
// Flashes color depending on direction headed
//**************************************************************
void ColorFlash(direction d)
{
  switch (d)
  {
    case EAST:
      lcd.setBacklight(RED);
      delay(500);
      lcd.setBacklight(WHITE);
      break;
    case WEST:
      lcd.setBacklight(GREEN);
      delay(500);
      lcd.setBacklight(WHITE);
      break;
    case NORTH:
      lcd.setBacklight(BLUE);
      delay(500);
      lcd.setBacklight(WHITE);
      break;
    case SOUTH:
      lcd.setBacklight(YELLOW);
      delay(500);
      lcd.setBacklight(WHITE);
      break;
    default:
      lcd.setBacklight(WHITE);
      break;
  }
}

//**************************************************************
// Encoder reader
//**************************************************************
void EncoderRead()
{
  int lEncVal = digitalRead(lEncPin);
  int rEncVal = digitalRead(rEncPin);
  
  if (lEncVal != lEncLast) lEncCount++;
  if (rEncVal != rEncLast) rEncCount++;

  lEncLast = lEncVal;
  rEncLast = rEncVal;

  //error = lEncCount - rEncCount;
}

//**************************************************************
// Refreshes the encoder count values
//**************************************************************
void RefreshCount()
{
  rEncCount = 0; //Counter for right encoder
  lEncCount = 0; //Counter for left encoder
}


//**************************************************************
// Reads each of the 3 sensors
//**************************************************************
void SensorRead()
{
    //Sensor Reading
    f = FrontSensor();
    l = LeftSensor();
    r = RightSensor();
}

//**************************************************************
// Takes the average of 1000 readings
//**************************************************************
float AverageSensorDistance(int sensor)
{
  float sensorDistance = 0;
  float totalDistance = 0;
  int totalReadings = 1000; 
  
  for(int i  = 0; i < totalReadings; i++)
  {
     totalDistance += analogRead(sensor);
  }

  totalDistance /= totalReadings;
  return totalDistance;
}

//**************************************************************
// Determines front sensor distance
//**************************************************************
float FrontSensor()
{
      FrontSensorValue = AverageSensorDistance(SFSensor);
      
      if((FrontSensorValue >= 573) && (FrontSensorValue < 607 ))    {return 2.00;}
      else if((FrontSensorValue >= 393) && (FrontSensorValue < 437)){return 3.00;}
      else if((FrontSensorValue >= 296) && (FrontSensorValue < 337)){return 4.00;}
      else if((FrontSensorValue >= 229) && (FrontSensorValue < 273)){return 5.00;}
      else if((FrontSensorValue >= 180) && (FrontSensorValue < 233)){return 6.00;}
      else if((FrontSensorValue >= 159) && (FrontSensorValue < 179)){return 7.00;}
      else if((FrontSensorValue >= 143) && (FrontSensorValue < 158)){return 8.00;}
      else if((FrontSensorValue >= 126) && (FrontSensorValue < 142)){return 9.00;}
      else if((FrontSensorValue >= 107) && (FrontSensorValue < 125)){return 10.00;}
      else if((FrontSensorValue < 106)){ return 11.00;}
}

//**************************************************************
// Determines right sensor distance
//**************************************************************
float RightSensor()
{
      RightSensorValue = AverageSensorDistance(SRSensor);
      
      if((RightSensorValue >= 510) && (RightSensorValue < 546 ))    {return 2.00;}
      else if((RightSensorValue >= 376) && (RightSensorValue < 395)){return 3.00;}
      else if((RightSensorValue >= 299) && (RightSensorValue < 316)){return 4.00;}
      else if((RightSensorValue >= 236) && (RightSensorValue < 255)){return 5.00;}
      else if((RightSensorValue >= 205) && (RightSensorValue < 219)){return 6.00;}
      else if((RightSensorValue >= 175) && (RightSensorValue < 191)){return 7.00;}
      else if((RightSensorValue >= 160) && (RightSensorValue < 174)){return 8.00;}
      else if((RightSensorValue >= 144) && (RightSensorValue < 159)){return 9.00;}
      else if((RightSensorValue >= 129) && (RightSensorValue < 143)){return 10.00;}
      else if((RightSensorValue < 128)){ return 11.00;}
}

//**************************************************************
// Determines left sensor distance
//**************************************************************
float LeftSensor()
{
      LeftSensorValue = AverageSensorDistance(SLSensor);
      
      if((LeftSensorValue >= 514) && (LeftSensorValue < 539 ))    {return 2.00;}
      else if((LeftSensorValue >= 379) && (LeftSensorValue < 406)){return 3.00;}
      else if((LeftSensorValue >= 296) && (LeftSensorValue < 318)){return 4.00;}
      else if((LeftSensorValue >= 241) && (LeftSensorValue < 259)){return 5.00;}
      else if((LeftSensorValue >= 202) && (LeftSensorValue < 219)){return 6.00;}
      else if((LeftSensorValue >= 176) && (LeftSensorValue < 192)){return 7.00;}
      else if((LeftSensorValue >= 155) && (LeftSensorValue < 173)){return 8.00;}
      else if((LeftSensorValue >= 138) && (LeftSensorValue < 154)){return 9.00;}
      else if((LeftSensorValue >= 123) && (LeftSensorValue < 137)){return 10.00;}
      else if((LeftSensorValue < 123)){ return 11.00;}
}

//**************************************************************
// Saturation function to allow for adjustment
//**************************************************************
float SaturationFunc(float Sensor, int KP, bool leftFlag)
{
  float saturation = 0.0;

  if(leftFlag)
  {
    saturation = (KP * (Sensor - 50) / 10) + STOP;
  }
  else
  {
    saturation = STOP - (KP *(Sensor - 50) / 10);
  }

  if (saturation < 1300)
    saturation = 1300;

  if (saturation > 1700)
    saturation = 1700;

  return saturation;    
}

//**************************************************************
// Prints message upon completion
//**************************************************************
void CompleteMessage()
{
  startFlag = false;
  LServo.detach();
  RServo.detach();
  
  while(true){   
    lcd.clear();
    lcd.print("CONGRATULATIONS!");
    lcd.setCursor(0,1);
    lcd.print(" GG EZ GIT GUD! ");
  }
}

