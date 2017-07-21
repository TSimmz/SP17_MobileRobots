#include "Maze.h"

//**************************************************************
// Creates the 9x9 array
//**************************************************************
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
  M = getMazeArray(size);  
  
  for (int i = 0; i < size; i++){
    for (int j = 0; j < size; j++)
    {
      if(i % 2 == 0 || j % 2 == 0)
        M[i][j] = COLOR;
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
// Resets all the VISITED positions to OPEN
//**************************************************************
void Maze::ResetToOpen()
{
  for (int i = 0; i < size; i++){
    for (int j = 0; j < size; j++){
      if(i % 2 != 0 && j % 2 != 0)
        M[i][j] = OPEN;
    }
  }
}
//**************************************************************
// Gets the state of the position
//**************************************************************
state Maze::getState(const Position &P) const
{
  return M[P.getCol()][P.getRow()];
}

//**************************************************************
// Sets the state of the position
//**************************************************************
void Maze::setState(const Position &P, state s)
{
  int i = P.getCol();
  int j = P.getRow();
  
  M[i][j] = s;
}
