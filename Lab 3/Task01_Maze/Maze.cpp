#include "Maze.h"


state **getMazeArray(int size) {
  state **A;
  
  A = new state *[size];
  for(int i = 0; i < size; i++)
      A[i] = new state[size];
      
  return A;
}

//**************************************************************
// Sets outer walls and Blue And Red Tape
//**************************************************************
Maze::Maze()
{
  size = MAZESIZE;
  start = Position(1,1); // Upper left corner position
  M = getMazeArray(size);  
  
  for (int i = 0; i < size; i++){
    for (int j = 0; j < size; j++)
    {
      if(i % 2 == 0 || j % 2 == 0)
        M[i][j] = RED;
       if(i % 2 != 0 && j % 2 != 0)
        M[i][j] = OPEN;
    }
  }
  for (int i = 0; i < size; i++){
    for (int j = 0; j < size; j++)
    {
      if(i == 0 || i == size - 1)
        M[i][j] = WALL;
      if(j == 0 || j == size - 1)
        M[i][j] = WALL;
    }
  }
}

//**************************************************************
// Initializes Maze #1
//**************************************************************
void Maze::init_Maze1() 
{
  // Set inner walls here
  M[1][2] = WALL; M[2][2] = WALL; M[3][2] = WALL;
  M[4][2] = WALL; M[6][2] = WALL; M[4][3] = WALL;
  M[6][3] = WALL; M[6][4] = WALL; M[2][5] = WALL;
  M[6][5] = WALL; M[2][6] = WALL; M[3][6] = WALL;
  M[4][6] = WALL; M[5][6] = WALL; M[6][6] = WALL;
}

//**************************************************************
// Initializes Maze #2
//**************************************************************
void Maze::init_Maze2()
{
  // Set inner walls here
  M[1][2] = WALL; M[2][2] = WALL; M[3][2] = WALL;
  M[4][2] = WALL; M[6][2] = WALL; M[4][3] = WALL;
  M[6][3] = WALL; M[2][4] = WALL; M[3][4] = WALL;
  M[4][4] = WALL; M[6][4] = WALL; M[2][5] = WALL;
  M[6][5] = WALL; M[2][6] = WALL; M[4][6] = WALL; 
  M[5][6] = WALL; M[6][6] = WALL;
}

//**************************************************************
// Initializes Maze #3
//**************************************************************
void Maze::init_Maze3()
{
  // Set inner walls here
  M[2][2] = WALL; M[3][2] = WALL; M[4][2] = WALL; 
  M[5][2] = WALL; M[6][2] = WALL; M[6][3] = WALL; 
  M[2][4] = WALL; M[6][4] = WALL; M[2][5] = WALL;  
  M[2][6] = WALL; M[3][6] = WALL; M[4][6] = WALL; 
  M[5][6] = WALL; M[6][6] = WALL;
}

//**************************************************************
// Gets the state of the position
//**************************************************************
state Maze::getState(const Position &P) const
{
  return M[P.getCol()][P.getRow()];
}

void Maze::setState(const Position &P, state s)
{
  int i = P.getCol();
  int j = P.getRow();
  
  M[i][j] = s;
}
