#include <string>
using namespace std;

struct BoardEntry {
  string player;
  string shape;
  short sq;
};
void fill(BoardEntry * squares[6], BoardEntry( & pieces)[4]) {
  for (BoardEntry & piece: pieces) {
    short sq = piece.sq;
    if (sq != -1)
      squares[sq] = & piece;
  }
}
int main() {
  BoardEntry * squares[6] = {
    nullptr
  };
  BoardEntry pieces[4] = {
    {
      "BLACK",
      "PAWN",
      0
    },
    {
      "BLACK",
      "KNIGHT",
      1
    },
    {
      "BLACK",
      "BISHOP",
      3
    },
    {
      "BLACK",
      "KING",
      4
    },
  };
  const BoardEntry * KING_IND[1] = {& pieces[3]};
  fill(squares, pieces);

  return 0;
}