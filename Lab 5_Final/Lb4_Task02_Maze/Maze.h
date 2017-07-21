#ifndef __MAZE
#define __MAZE

#include "Position.h"

const Position NULLPOS(-1,-1);

using namespace std;

typedef enum{OPEN, WALL, VISITED, COLOR, UPOS} state;

state **getMazeArray(int size);

class Maze
{
  public:
   static const int MAZESIZE = 9; // default maze size
 
   Maze();

   void ResetToOpen();
  
   state getState(const Position &P) const;
   void setState(const Position &P, state s);
   
   int size;
   Position start;
   Position exitPos;
   state **M;         // two-dimensional array of state values
};

#endif
