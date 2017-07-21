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
bool DFS = false;
bool BFS = false;
bool BFS_start = false;

int KP = 8;
int error = 0;

//Compass, Current Position, and Final Path stack
Position currPos;
direction compass;

StackArray<Position> FinalPath;

StackArray<direction> Heading;
StackArray<Position> Path;

//Holds the starting and ending grid position
int startPos, endPos; 

// Holds the returned distance value
int f, l, r;              

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
  compass = EAST;
  currPos = Position(1,1);
  pos[0] = 1;            //Sets initial grid location to 1
   
	Path.push(currPos);    //Pushes current position onto stack
  Heading.push(compass); //Pushes current heading onto stack
 
	//Setting up a friendly message
  lcd.setBacklight(WHITE);
  lcd.setCursor(0,0);
  lcd.print("Robots-Lb3-T1");
  delay(1000);
}

//**************************************************************
// Main execution loop
//**************************************************************
void loop() 
{
  if (!DFS && !BFS)       //If a choice needs to be made
  {
    ChooseDFSorBFS();
  }
  else if (DFS)           //If Mapping Maze
  {
    Display();
    MazeMapper();         // Maps the maze

    if (isComplete())
    {
      PositionScan();     // Gets last positions walls
      lcd.clear();
      lcd.print("Maze mapped!");
      
      DFS = false;        // Maze mapping is done
      BFS_start = true;   //BFS enabled
      startFlag = false;  //Choose loop enabled
      delay(1000);
    }
  }
  else if (BFS && startFlag) //If Finding shortest path
  {
    Display();
    Movement();

    if(currPos.getPositionGrid(currPos) == endPos)
    {
      CompleteMessage(0);   
    }
  }
}

//**************************************************************
// Allows user to start the maze mapper or bfs
//**************************************************************
void ChooseDFSorBFS()
{
  lcd.clear();
  lcd.print("L: Map maze");
  lcd.setCursor(0,1);
  lcd.print("R: Shortest Path");
  
  while(!startFlag)
  {
    buttons = lcd.readButtons();
    if(buttons)
    {
      lcd.setCursor(0,0);
  
      if (buttons & BUTTON_LEFT) 
      {
          lcd.clear();
          lcd.print("1. Map the Maze");
          options = 1;
      }
      else if (buttons & BUTTON_RIGHT)
      {
        if(BFS_start){
          lcd.clear();
          lcd.print("2. Shortest Path");
          options = 2;
        }
        else{
          options = 0;
          lcd.clear();
          lcd.print("ERR: Must map");
          lcd.setCursor(0,1);
          lcd.print("the maze first.");
        }
      }
      else if (buttons & BUTTON_SELECT)
      {
        switch (options)
        {
          case 1:
            DFS = true;
            startFlag = true;
            break;
          case 2:
            startFlag = true;
            BFS_Setup();
            break;
          default:
            lcd.clear();
            lcd.print("Please select");
            lcd.setCursor(0,1);
            lcd.print("an option");
            break;
        }
        options = 0;
      }
      else{}
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
// Similar to DFS - this maps the maze initially
//**************************************************************
void MazeMapper()
{
  int i, j, n = 0, t = 0;
  direction d, e, f;
  Position nbr1, nbr2;

  if (maze->getState(currPos) != VISITED)
  {
    for (f = SOUTH, i = 1; i <= 4; f = next_dirR(f), i++)
    {
      nbr1 = currPos.Neighbor(f, 1);
      if (pos[i] == 1)
        maze->setState(nbr1, WALL);
    }
    
    maze->setState(currPos, VISITED);
  }else{}
  
  for (d = compass, i = 0; i < 4; d = next_dirR(d), i++)
  {
    nbr1 = currPos.Neighbor(d, 1);          //Checks immediate neighbor
    if (maze->getState(nbr1) == WALL)
      n++;
    else{
      nbr2 = currPos.Neighbor(d, 2);        //Checks neighbor's neighbor
      if (maze->getState(nbr2) == OPEN){
        Turn(n);                            // Turns 90 degrees, n times;
        currPos = nbr2;                     // Updates position
        MoveForward();                      // Moves forward     
        return;
      }
      else
        n++;
    }
  }
  if(i == 4)  //If above loop did not find a space
  {    
    e = opposite_dir(Heading.peek());
    while(e != d)
    {
      d = next_dirR(d);
      t++;
    }
    Heading.pop();
    Path.pop();
    
    Turn(t);
    MoveForward();
    Path.pop();
    Heading.pop();
    
    currPos = Path.peek();
    return;
  }
}

//**************************************************************
// Performs all display functions to reduce redundant code
//**************************************************************
void Display()
{
  PositionScan();     // Sets values in position array
  PositionDisplay();  // Displays heading on LCD
  GridDisplay();      // Displays array of grid positions
}

//**************************************************************
// Setups for the breadth-first search
//**************************************************************
void BFS_Setup()
{
  maze->ResetToOpen();
  Init_Position();    //Sets initial start, end and compass

  pos[0] = startPos;
  maze->start = maze->start.getGridPosition(startPos);
  maze->exitPos = maze->exitPos.getGridPosition(endPos);

  int rc = FindShortestPath();     //Performs the BFS algo to find the shortest path
  
  if (rc != 0 || FinalPath.isEmpty())
      CompleteMessage(rc);
  else
  {
     BFS = true;
     currPos = FinalPath.peek(); //sets current position from finalpath queue     
     FinalPath.pop();
  }
}

//**************************************************************
// Finds the shortest path in the mapped maze
//**************************************************************
int FindShortestPath()
{
  int k;
  QueueArray<Position> Q;
  Position current;
  Position nbr1;
  Position nbr2;
  direction d;

	current = maze->start;  
	maze->setState(current, VISITED);       // Sets our current location as visited

	Q.enqueue(current);
  
	while(!(Q.isEmpty()))
	{
    //Calculating();		
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
  int rSpeed = STOP - 63;  //Adjust this speed
  
  ColorFlash(compass);
  
  while(lEncCount < 140 && rEncCount < 140)
  {
    LServo.write(lSpeed); 
    RServo.write(rSpeed); 
    EncoderRead();
  }
  pos[0] = GetGrid(pos[0]);

  if(DFS){
    Heading.push(compass);
    Path.push(currPos);
  }
   
  Stop();
}

//**************************************************************
// Turns clockwise 90 degrees for every value of turn, unless
//  turns == 3, then turns counter clock-wise 90
//**************************************************************
void Turn(int turns)
{
  int tCount = 27;
  
  if (turns == 0)
    return;
	
	if (turns % 3 == 0)
	{
		RefreshCount();
    while(lEncCount < tCount && rEncCount < tCount)
    {
      LServo.write(STOP-70);
      RServo.write(STOP-70);
      EncoderRead();
    }
    compass = next_dirL(compass);
    Stop();
    delay(250);
	
	}
  else
	{
		for(int i = 0; i < turns; i++)
		{
			RefreshCount();
			while(lEncCount < tCount && rEncCount < tCount)
			{
				LServo.write(STOP+70);
				RServo.write(STOP+70);
				EncoderRead();
			}
			compass = next_dirR(compass);
			Stop();
			delay(250); 
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
	// Remove parameter before testing
	/*for(int i = 0; i < 100; i++)
	{
		if(FrontSensor() <= 10.0){
			f = 1;
			break;
		}
		f = 0;
	}
	for(int i = 0; i < 100; i++)
	{
		if(LeftSensor() <= 10.0){
			l = 1;
			break;
		}
		l = 0;
	}
	for(int i = 0; i < 100; i++)
	{
		if(RightSensor() <= 10.0){
			r = 1;
			break;
		}
		r = 0;
	}*/
  return (sensor <= 10.00) ? 1 : 0;
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
	 
	//WallScan()
	//Set each equal to f, l, or r individually
	
	SensorRead(); //Remove this
  
  switch(compass)
  {
    case EAST:
      pos[1] = WallScan(r); //SOUTH Wall/Open
      pos[2] = -1;          //WEST always unknown
      pos[3] = WallScan(l); //NORTH Wall/Open
      pos[4] = WallScan(f); //EAST Wall/Open
      break;
    case WEST: 
      pos[1] = WallScan(l); //SOUTH Wall/Open
      pos[2] = WallScan(f); //WEST Wall/Open
      pos[3] = WallScan(r); //NORTH Wall/Open
      pos[4] = -1;          //EAST always unknown
      break;
    case SOUTH:  
      pos[1] = WallScan(f); //SOUTH Wall/Open
      pos[2] = WallScan(r); //WEST Wall/Open
      pos[3] = -1;          //NORTH always unknown
      pos[4] = WallScan(l); //EAST Wall/Open
      break;
    case NORTH:
      pos[1] = -1;          //SOUTH always unknown
      pos[2] = WallScan(l); //WEST Wall/Open
      pos[3] = WallScan(f); //NORTH Wall/Open
      pos[4] = WallScan(r); //EAST Wall/Open
      break;
  }
  delay(500);
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
  }
  
  if(count == 15)
  {
    for(int i = 0; i < 16; i++)   //Resets grid array
      grid[i] = 'O';
        
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
// Gets the current grid position to display
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
      int FrontSensorValue = AverageSensorDistance(SFSensor);
      
      if(FrontSensorValue >= 573)                                   {return 2.00;}
      else if((FrontSensorValue >= 393) && (FrontSensorValue < 437)){return 3.00;}
      else if((FrontSensorValue >= 296) && (FrontSensorValue < 337)){return 4.00;}
      else if((FrontSensorValue >= 229) && (FrontSensorValue < 273)){return 5.00;}
      else if((FrontSensorValue >= 180) && (FrontSensorValue < 233)){return 6.00;}
      else if((FrontSensorValue >= 159) && (FrontSensorValue < 179)){return 7.00;}
      else if((FrontSensorValue >= 143) && (FrontSensorValue < 158)){return 8.00;}
      else if((FrontSensorValue >= 126) && (FrontSensorValue < 142)){return 9.00;}
      else if((FrontSensorValue >= 101) && (FrontSensorValue < 125)){return 10.00;}
      else if((FrontSensorValue < 100)){ return 11.00;}
}

//**************************************************************
// Determines right sensor distance
//**************************************************************
float RightSensor()
{
      int RightSensorValue = AverageSensorDistance(SRSensor);
      
      if(RightSensorValue >= 510)                                   {return 2.00;}
      else if((RightSensorValue >= 376) && (RightSensorValue < 395)){return 3.00;}
      else if((RightSensorValue >= 299) && (RightSensorValue < 316)){return 4.00;}
      else if((RightSensorValue >= 236) && (RightSensorValue < 255)){return 5.00;}
      else if((RightSensorValue >= 205) && (RightSensorValue < 219)){return 6.00;}
      else if((RightSensorValue >= 175) && (RightSensorValue < 191)){return 7.00;}
      else if((RightSensorValue >= 160) && (RightSensorValue < 174)){return 8.00;}
      else if((RightSensorValue >= 144) && (RightSensorValue < 159)){return 9.00;}
      else if((RightSensorValue >= 121) && (RightSensorValue < 143)){return 10.00;}
      else if((RightSensorValue < 120)){ return 11.00;}
}

//**************************************************************
// Determines left sensor distance
//**************************************************************
float LeftSensor()
{
      int LeftSensorValue = AverageSensorDistance(SLSensor);
      
      if(LeftSensorValue >= 514)                                  {return 2.00;}
      else if((LeftSensorValue >= 379) && (LeftSensorValue < 406)){return 3.00;}
      else if((LeftSensorValue >= 296) && (LeftSensorValue < 318)){return 4.00;}
      else if((LeftSensorValue >= 241) && (LeftSensorValue < 259)){return 5.00;}
      else if((LeftSensorValue >= 202) && (LeftSensorValue < 219)){return 6.00;}
      else if((LeftSensorValue >= 176) && (LeftSensorValue < 192)){return 7.00;}
      else if((LeftSensorValue >= 155) && (LeftSensorValue < 173)){return 8.00;}
      else if((LeftSensorValue >= 138) && (LeftSensorValue < 154)){return 9.00;}
      else if((LeftSensorValue >= 117) && (LeftSensorValue < 137)){return 10.00;}
      else if((LeftSensorValue < 116)){ return 11.00;}
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
    delay(250);
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
