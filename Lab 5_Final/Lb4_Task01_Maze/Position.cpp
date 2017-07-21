#include "Position.h"

//const Position NULLPOS(-1,-1);

using namespace std;

//Position **getPredArray(int size){
//  Position **pred;
//  pred = new Position *[size];
//  for(int i = 0; i < size; i++)
//    pred[i] = new Position[size];
//  
//  return pred;
//}
//
//Position::Position()
//{
//  Pred = getPredArray(9);
//
//  for(int i = 0; i < 9; i++)
//  {
//    for(int j = 0; i < 9; i++)
//    {
//      Pred[i][j] = NULLPOS;
//    }
//  }
//}

//Position Position::getPos(const Position &P) const
//{
//  return Pred[P.getCol()][P.getRow()];
//}
//
//void Position::setPos(const Position &P, const Position &Q)
//{
//  Pred[P.getCol()][P.getRow()] = Q;
//}

direction next_dirR(direction d)
{
   switch(d)
   {
      case SOUTH: return WEST;
      case WEST: return NORTH;
      case NORTH: return EAST;
      case EAST: return SOUTH;
   }
}

direction next_dirL(direction d)
{
   switch(d)
   {
      case SOUTH: return EAST;
      case WEST: return SOUTH;
      case NORTH: return WEST;
      case EAST: return NORTH;
   }
}

direction next_direction(direction d)
{
  switch(d)
  {
    case EAST: return SOUTH;
    case SOUTH: return WEST;
    case WEST: return NORTH;
    case NORTH: return NONE;
    
  }
}

direction opposite_dir(direction d)
{
  switch(d)
  {
    case EAST: return WEST;
    case SOUTH: return NORTH;
    case WEST: return EAST;
    case NORTH: return SOUTH;
  }
}

int Position::getPositionGrid(const Position &P) const
{
  if(P == Position(1,1))
    return 1;
  else if(P == Position(3,1))
    return 2;
  else if(P == Position(5,1))
    return 3;
  else if(P == Position(7,1))
    return 4;
  else if(P == Position(1,3))
    return 5;
  else if(P == Position(3,3))
    return 6;
  else if(P == Position(5,3))
    return 7;
  else if(P == Position(7,3))
    return 8;
  else if(P == Position(1,5))
    return 9;
  else if(P == Position(3,5))
    return 10;
  else if(P == Position(5,5))
    return 11;
  else if(P == Position(7,5))
    return 12;
  else if(P == Position(1,7))
    return 13;
  else if(P == Position(3,7))
    return 14;
  else if(P == Position(5,7))
    return 15;
  else if(P == Position(7,7))
    return 16;
}

Position Position::getGridPosition(int grid) const
{
	switch(grid)
	{
		case 1: return Position(1,1);
		case 2: return Position(3,1);
		case 3: return Position(5,1);
		case 4: return Position(7,1);
		case 5: return Position(1,3);
		case 6: return Position(3,3);
		case 7: return Position(5,3);
		case 8: return Position(7,3);
		case 9: return Position(1,5);
		case 10:return Position(3,5);
		case 11:return Position(5,5);
		case 12:return Position(7,5);
		case 13:return Position(1,7);
		case 14:return Position(3,7);
		case 15:return Position(5,7);
		case 16:return Position(7,7);
	}
}

Position Position::Neighbor(direction d, int i) const
{
   switch(d) {
      case SOUTH:
	      return Position(col,row+i);
      case WEST:
	      return Position(col-i,row);
      case NORTH:
	      return Position(col,row-i);
      case EAST:
	      return Position(col+i,row);
   }
}

bool Position::operator==(const Position &other) const
{
   return row == other.row && col == other.col;
}
