#include "Position.h"

using namespace std;

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
