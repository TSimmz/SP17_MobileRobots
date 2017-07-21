#ifndef __POSITION
#define __POSITION

using namespace std;


typedef enum{EAST, SOUTH, WEST, NORTH, NONE} direction;

direction next_dirL(direction d);
direction next_dirR(direction d);
direction next_direction(direction d);
direction opposite_dir(direction d);
//int getPositionGrid(const Position &P) const;


class Position {
  public:
   //Position();
   Position():row(0),col(0) {};
   Position(int c, int r):col(c), row(r) {};
   int getRow() const { return row;}
   int getCol() const { return col;}
  
   Position Neighbor(direction d, int i) const;
   Position getGridPosition(int grid) const;

   int getPositionGrid(const Position &P) const;
   
   //Position getPos(const Position &P) const;
   //void setPos(const Position &P, const Position &Q);

   bool operator==(const Position &other) const;

  private:
   int row;
   int col;
   Position **Pred;
};
#endif
