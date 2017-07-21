#ifndef __MAZE
#define __MAZE

#include "Position.h"

const Position NULLPOS(-1,-1);

using namespace std;

typedef enum{OPEN, WALL, VISITED, COLOR} state;

state **getMazeArray(int size);
//Position **getPredArray(int size);

class Maze
{
  public:
   static const int MAZESIZE = 9; // default maze size
 
   Maze();

   void init_Maze1(); 
   void init_Maze2(); 
   void init_Maze3(); 
  
   state getState(const Position &P) const;
   void setState(const Position &P, state s);

   //void print_dfsExitPath();
   
   int size;
   Position start;
   Position exitPos;
   state **M;         // two-dimensional array of state values
};

//void printPredecessorPath(Position **pred, Position target);
#endif
