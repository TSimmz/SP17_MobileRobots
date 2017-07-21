//Library codes
#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>
#include <Servo.h>
#include <stdlib.h>

#include "Maze.h"
#include "Position.h"
#include "StackArray.h"
#include "QueueArray.h"

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
int prev[17] = {0};       // Hold previous positions
int pos[5] = {0};         // U: -1, O: 0, W: 1
int options = 0;          // Options for maze choice
bool startFlag = false;   // Flag for starting movement

int KP = 8;
int error = 0;

//Compass, Current Position, and Final Path stack
direction compass;
Position currPos;
StackArray<Position> FinalPath;

//Holds the starting and ending grid position
int startPos, endPos; 

// Holds the returned distance value
int f, l, r;              
int FrontSensorValue, RightSensorValue, LeftSensorValue;

//Declares the maze
Maze* maze;

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

  // Setting the encoders
  pinMode(lEncPin, INPUT_PULLUP);
  pinMode(rEncPin, INPUT_PULLUP);
	
	maze = new Maze();     //Creates the borders of the maze
	
	//Setting up a friendly message
  lcd.setBacklight(WHITE);
  lcd.setCursor(0,0);
  lcd.print("Robots-Lb3-T1");
   
  MazeSelect();          //Select which maze to run
	Init_Position();       //Sets initial start, end and compass

  pos[0] = startPos;
	maze->start = maze->start.getGridPosition(startPos);
	maze->exitPos = maze->exitPos.getGridPosition(endPos);
  
	int rc = FindShortestPath();		 //Performs the BFS algo to find the shortest path
  if (rc != 0)
    CompleteMessage(rc);

  if(FinalPath.isEmpty())
    CompleteMessage(-1);
  else
  {
	  currPos = FinalPath.peek(); //sets current position from finalpath queue
    FinalPath.pop();
  }
}

//**************************************************************
// Main execution loop
//**************************************************************
void loop() 
{
  PositionScan();     // Sets values in position array
	PositionDisplay();  // Displays heading on LCD
	GridDisplay();      // Displays array of grid positions
	
	Movement();
  
  if(currPos.getPositionGrid(currPos) == endPos)
  {
    CompleteMessage(0);
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
// Allows user to initialize start, end, and compass heading
//**************************************************************
void Init_Position()
{
	//Set starting Grid location
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Please select");
  lcd.setCursor(0,1);
  lcd.print("Start Grid");
  delay(1000);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Start Grid");
  startPos = SetGrid();
  
  //Set ending Grid location
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Please select");
  lcd.setCursor(0,1);
  lcd.print("End Grid");
  delay(1000);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("End Grid");
  endPos = SetGrid();

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Start Grid: ");
  lcd.setCursor(13,0);
  lcd.print(startPos);


  lcd.setCursor(0,1);
  lcd.print("End Grid: ");
  lcd.setCursor(13,1);
  lcd.print(endPos);
  delay(1000);

  SetOrientation();
}

//**************************************************************
// Chooses and moves next direction based on maze
//**************************************************************
int FindShortestPath()
{
  QueueArray<Position> Q;
  Position current, nbr1, nbr2;
	int k;
  direction d;
  	
	current = maze->start;
	maze->setState(current, VISITED);       // Sets our current location as visited

	Q.enqueue(current);  
	while(!(Q.isEmpty()))
	{
    Calculating();		
		current = Q.dequeue();
		for(k = 0, d = compass; k < 4; d = next_dirR(d), k++)
		{
			nbr1 = current.Neighbor(d, 1);          //Checks immediate neighbor
			if (maze->getState(nbr1) == COLOR)
			{
				nbr2 = current.Neighbor(d, 2);        
				if (maze->getState(nbr2) == OPEN)			//Checks neighbor's neighbor
				{
					maze->setState(nbr2, VISITED);
					Q.enqueue(nbr2);
                    
					prev[nbr2.getPositionGrid(nbr2)] = current.getPositionGrid(current);
					
					if(nbr2.getPositionGrid(nbr2) == endPos)
					{
						PredecessorPath();
            Countdown();
						return 0;
					}
				}
			}
		}
	}
  return -1;
}
//**************************************************************
// Prints the predecessor path of the robot
//**************************************************************	
void PredecessorPath()
{
  Position temp;
  int curr = endPos;
  int next = 0;

  temp = temp.getGridPosition(curr);
  FinalPath.push(temp);
  while(curr != startPos)
  {
    next = prev[curr];
    temp = temp.getGridPosition(next);
    FinalPath.push(temp);
    curr = next;
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
  int rSpeed = STOP - 68;  //Adjust this speed
  
  ColorFlash(compass);
  
  while(lEncCount < 140 && rEncCount < 140)
  {
    LServo.write(lSpeed); 
    RServo.write(rSpeed); 
    EncoderRead();
  }
  pos[0] = GetGrid(pos[0]);  
  Stop();
}

//**************************************************************
// Turns clockwise 90 degrees for every value of turn, unless
//  turns == 3, then turns counter clock-wise 90
//**************************************************************
void Turn(int turns)
{
  if (turns == 0)
    return;
	
	if (turns % 3 == 0)
	{
		RefreshCount();
    while(lEncCount < 27 && rEncCount < 27)
    {
      LServo.write(STOP-70);
      RServo.write(STOP-70);
      EncoderRead();
    }
    compass = next_dirL(compass);
    Stop();
    delay(500);
	
	}
  else
	{
		for(int i = 0; i < turns; i++)
		{
			RefreshCount();
			while(lEncCount < 27 && rEncCount < 27)
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
// Moves the robot from start to finish once BFS is complete
//**************************************************************
void Movement()
{
	Position nextPos = FinalPath.peek();
  FinalPath.pop();
	direction d = compass;
	direction temp;
	int t = 0;
	
	if(currPos.getCol() == nextPos.getCol())
	{
		if(currPos.getRow() < nextPos.getRow()) //Robot needs to move South
		{
			temp = SOUTH;
		}
		else																		//Robot needs to move North
		{
			temp = NORTH;
		}
	}
	else if(currPos.getCol() < nextPos.getCol())//Robot needs to move East
	{
		temp = EAST;
	}
	else																			//Robot needs to move west		
	{
		temp = WEST;
	}
	
	while(d != temp)
	{
		d = next_dirR(d);
		t++;
	}
	Turn(t);
	MoveForward();
	currPos = nextPos;
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
   
  // Add loop around this to make sure that each reading DEFINITELY sees a wall or not
  // Possibly extend timer delay afterwards
  
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
  //Replace O's in grid array with X's as new grids are visited
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
// Handles neighbor assigment - for use in Task 2
//**************************************************************
void NeighborAssign()
{
	for(int i = 0; i < sizeof(pos); i++)
	{
		if(pos[i] == 1)
		{
			//assign neighbor postion to WALL
		}
		else if(pos[i] == 0)
		{
			//assign neighbor position to COLOR
			//assign neighbors neighbor position to OPEN
		}
	}
	delay(1000);
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
// Allows user to set start and end positions
//**************************************************************
int SetGrid()
{
  int i = 0;
  bool end = false;
  while(!end)
  {
    buttons = lcd.readButtons();
    if (buttons)
    {
      delay(100);
      lcd.setCursor(0,1);
      lcd.print(i);
      
      if (buttons & BUTTON_UP)
      {
        delay(100);
        i++;
        if (i > 16)
        {
          i = 1;
          lcd.setCursor(1,1);
          lcd.print(" ");
        }
        lcd.setCursor(0,1);
        lcd.print(i);
      }

      if (buttons & BUTTON_DOWN)
      {
        delay(100);
        i--;
        if(i <= 0)
          i = 16;
				
        if (i < 10)
        {
          lcd.setCursor(1,1);
          lcd.print(" ");
        }
        lcd.setCursor(0,1);
        lcd.print(i);
      }

      if (buttons & BUTTON_SELECT)
      {
        i = (i == 0) ? 1 : i;
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Grid Selected:");
        lcd.setCursor(0,1);
        lcd.print(i);
        delay(1000);
        end = true;
        return i;
      }
    }
  }
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
// Allows user to set the orientation of robot
//**************************************************************
void SetOrientation()
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Please select");
  lcd.setCursor(0,1);
  lcd.print("Orientation");
  delay(1000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Orientation:");
  bool end = false;
  int option;
  
  while(!end)
  {
    buttons = lcd.readButtons();
    if (buttons)
    {
      delay(100);
      lcd.setCursor(0,1);
      if (buttons & BUTTON_UP)
      {
        lcd.print("NORTH");
        option = 0;
      }

      if (buttons & BUTTON_RIGHT)
      {
        lcd.print("EAST ");
        option = 1;
      }

      if (buttons & BUTTON_DOWN)
      {
        lcd.print("SOUTH");
        option = 2;
      }

      if (buttons & BUTTON_LEFT)
      {
        lcd.print("WEST ");
        option = 3;
      }

      if (buttons & BUTTON_SELECT)
      {
				lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Facing: ");
				lcd.setCursor(0,1);
        switch(option)
        {
          case 0:
						lcd.print("NORTH");
            compass = NORTH;
            return;
          case 1:
						lcd.print("EAST ");
            compass = EAST;
            return;
          case 2:
						lcd.print("SOUTH");
            compass = SOUTH;
            return;
          case 3:
						 lcd.print("WEST ");
            compass = WEST;
            return;
          default:
						lcd.print("Default: EAST ");
            compass = EAST;
            return;
        }
				end = true;
      }
    }
  }
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
// Outputs calculating message to user during FindShortestPath
//**************************************************************
void Calculating()
{
  lcd.clear();
  lcd.print("Calculating.");
  delay(500);
  lcd.clear();
  lcd.print("Calculating..");
  delay(500);
  lcd.clear();
  lcd.print("Calculating...");
  delay(500);
  lcd.clear();
  lcd.print("Calculating....");
  delay(500);
  lcd.clear();
}

//**************************************************************
// Outputs countdown to user before moving through the maze
//**************************************************************
void Countdown()
{
  int i = 5;
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print("Path Found!");
  delay(1000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Starting Maze In:");
  lcd.setCursor(0,1);
  while(i > 0)
  {
    lcd.print(i);
    lcd.print("..");
    i--;
    delay(1000);
    
  }
}
//**************************************************************
// Prints message upon completion
//**************************************************************
void CompleteMessage(int check)
{
  startFlag = false;
  LServo.detach();
  RServo.detach();
  
  if(check == 0)
  {
    while(true){   
      lcd.clear();
      lcd.print("CONGRATULATIONS!");
      lcd.setCursor(0,1);
      lcd.print(" GG EZ GIT GUD! ");
      delay(2000);
      lcd.clear();
      PositionDisplay();
      GridDisplay();
      delay(2000);
      
    }
  }
  else
  {
    lcd.clear();
    while(true){
      lcd.print("    ERR 404:    ");
      lcd.setCursor(0,1);
      lcd.print(" Path Not Found ");
      delay(1000);
    }
  }
}
