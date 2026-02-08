#include <cmath>
#include <cctype>
#include <utility>
#include <vector>

// remove in VEXIQ
#include <iostream>
#include <iomanip>

using namespace std;

char board[8][8] = {
    {'r', 'n', 'b', 'k', 'q', 'b', 'n', 'r'},
    {'o', 'o', 'o', 'o', 'o', 'o', 'o', 'o'},
    {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
    {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
    {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
    {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
    {'O', 'O', 'O', 'O', 'O', 'O', 'O', 'O'},
    {'R', 'N', 'B', 'K', 'Q', 'B', 'N', 'R'}
};
/*
0: empty
uppercase: white player, lowercase: black player
K/k: king, Q/q: queen, R/r: rook, B/b: bishop, N/n: knight, O/o: unused pawn (can step 2), P/p: used pawn
*/

// NMOVES: moves relative to the current coordinate
const pair<int,int> K_NMOVES[8] = { {1,0}, {1,1}, {0,1}, {-1,1}, {-1,0}, {-1,-1}, {0,-1}, {1,-1} };
const pair<int,int> N_NMOVES[8] = { {2,1}, {1,2}, {-1,2}, {-2,1}, {-2,-1}, {-1,-2}, {1,-2}, {2,-1} };

// NDIRS: move directions normalized and relative to the current coordinate
const pair<int,int> R_NDIRS[4] = { {1,0}, {-1,0}, {0,1}, {0,-1} };
const pair<int,int> B_NDIRS[4] = { {1,1}, {1,-1}, {-1,1}, {-1,-1} };
const pair<int,int> Q_NDIRS[8] = { {1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1} };

/*
isupper() is used to query the player on a given tile. 0: black player, !0: white player.
Note that white player can be ANY non-zero integer, not just 1.
Note that 0 can be either black or ' ', so a check for ' ' is required.
*/

inline int rel_foward(int player)
/**
 * @return the foward direction relative to given player.
 * In the configuration of white at bottom and black at top, white's foward = -1, black's foward = +1.
 */
{
    return (player) ? -1 : 1;
}

inline int sign(int n)
{
    return (n > 0) - (n < 0);
}

inline void move(char &tile_i, char &tile_f)
/// TODO: pawn promotion
{    
    if (tolower(tile_i) == 'o')
        tile_f = tile_i + 1;  // switch to used pawn
    
    else         
        tile_f = tile_i;
    
    tile_i = ' ';
}

void out_board()
{
    cout << "   ";
    for (int x = 0; x < 8; x++)
        cout << setw(3) << x;

    cout << endl << "  +-----------BOT------------+" << endl;
    for (int y = 0; y < 8; y++)
    {
        cout << y << setw(2) << "|";
        for (int x = 0; x < 8; x++)
            cout << setw(3) << board[y][x];

        cout << setw(3) << "|" << endl;
    }
    cout << "  +-----------YOU------------+" << endl;
}

inline bool is_movable(int player, int x, int y)
/**
 * @return whether the given tile is inside the board && is (empty || opponent of the given player).
 */
{
    return 0 <= x && x <= 7 && 0 <= y && y <= 7 && (board[y][x] == ' ' || isupper(board[y][x]) != player);
}

int validate(int x_i, int y_i, int x_f, int y_f)
/**
 * @return whether move is valid.
 * 0: valid, 1: broken chess rule, 2: empty tile, 3: out of board || friendly fire, 4: blocked path
 */
///TODO: 5: impersonating bot
{
    int piece_i = tolower(board[y_i][x_i]), player_i = isupper(board[y_i][x_i]),
        tile_f = board[y_f][x_f],
        dx = abs(x_f - x_i), dy = abs(y_f - y_i);

    if (piece_i == ' ')
        return 2;

    if (!is_movable(player_i, x_f, y_f))
        return 3;

    // king && check direction
    if (piece_i == 'k' && max(dx, dy) > 1)
        return 1;

    // queen && check direction
    if (piece_i == 'q' && dx && dy && dx != dy)
        return 1;

    // rook && check direction
    if (piece_i == 'r' && dx && dy)
        return 1;
    
    // bishop && check direction
    if (piece_i == 'b' && dx != dy)
        return 1;

    // knight && check direction
    if ((piece_i == 'n') && (dx != 1 || dy != 2) && (dx != 2 || dy != 1))
        return 1;

    // both pawns
    if (piece_i == 'p' || piece_i == 'o')
    {
        if (rel_foward(player_i) * (y_f - y_i) < 0) // check going backward
            return 1;

        // not ((piece_f != ' ' && dx == 1 && dy == 1)  ||  (piece_f == ' ' && dx == 0))
        if ((tile_f != ' ' || dx != 0) && (tile_f == ' ' || dy != 1 || dx != 1))
            return 1;

        if (piece_i == 'p' && dy > 1)
            return 1;

        if (piece_i == 'o' && dy > 2)
            return 1;
    }

    if (piece_i != 'k')
    {
        // check if all tiles on the path are empty
        int ndx = sign(x_f - x_i),
            ndy = sign(y_f - y_i);

        for (int x = x_i + ndx, y = y_i + ndy; x != x_f || y != y_f; x += ndx, y += ndy)
        {
            if (board[y][x] != ' ')
                return 4;
        }
    }

    // if all valid
    return 0;
}

vector<pair<int, int>> gen_moves(int player, char piece, int x_i, int y_i)
{
    vector<pair<int, int>> moves;
    int x = 0, y = 0, ndx = 0, ndy = 0;

    if (piece == 'k')
    {
        for (pair<int, int> nxy: K_NMOVES)
        {
            x = x_i + nxy.first, y = y_i + nxy.second;
            if (is_movable(player, x, y))
                moves.push_back({x, y});
        }
    }
    else if (piece == 'n')
    {
        for (pair<int, int> nxy: N_NMOVES)
        {
            x = x_i + nxy.first, y = y_i + nxy.second;
            if (is_movable(player, x, y))
                moves.push_back({x, y});
        }
    }
    else if (piece == 'p' || piece == 'o')
    {
        y = y_i + rel_foward(player);
        
        /// TODO: remove once added promotion
        if (y < 0 || 7 < y)
            return moves;

        // capture moves
        for (int ndx: {-1, 1})
        {
            x = x_i + ndx;
            if (0 <= x && x <= 7 && board[y][x] != ' ' && isupper(board[y][x]) != player)
                moves.push_back({x, y});
        }

        // y-moves
        if (board[y][x_i] == ' ')  // y-check done at the top
        {
            moves.push_back({x_i, y});
        
            if (piece == 'o')
            {
                y += rel_foward(player);
                if (board[y][x_i] == ' ')
                    moves.push_back({x_i, y});
            }
        }
    }
    else if (piece == 'q')
    {
        for (pair<int, int> ndxy: Q_NDIRS)
        {
            ndx = ndxy.first, ndy = ndxy.second;
            for (x = x_i + ndx, y = y_i + ndy; is_movable(player, x, y); x += ndx, y += ndy)
            {
                moves.push_back({x, y});
            }
        }
    }
    else if (piece == 'r')
    {
        for (pair<int, int> ndxy: R_NDIRS)
        {
            ndx = ndxy.first, ndy = ndxy.second;
            for (x = x_i + ndx, y = y_i + ndy; is_movable(player, x, y); x += ndx, y += ndy)
            {
                moves.push_back({x, y});
            }
        }
    }
    else if (piece == 'b')
    {
        for (pair<int, int> ndxy: B_NDIRS)
        {
            ndx = ndxy.first, ndy = ndxy.second;
            for (x = x_i + ndx, y = y_i + ndy; is_movable(player, x, y); x += ndx, y += ndy)
            {
                moves.push_back({x, y});
            }
        }
    }

    return moves;
}

bool is_checkmate()
/**
 * @return given player wins.
 *-1: loss, 0: tie, 1: win 
 */
{
    return 0;
}

int recur(int player)
{
    for (int y = 0; y < 8; y++)
    {
        for (int x = 0; x < 8; x++)
        {
            char tile = board[y][x];

            if (tile != ' ' && isupper(tile) == player)
            {
                for (pair<int, int> xy_f: gen_moves(player, tolower(tile), x, y))
                {
                    recur(!player);
                }
            }
        }
    }

    return 0;
}

int main()
{
    int x_i = 0, y_i = 0, x_f = 0, y_f = 0, invalid = 0;
    vector<pair<int, int>> xy_f;

    while (true)
    {
        out_board();
        do
        {
            cout << "Initial and final tile (x_i y_i x_f y_f): ";
            cin >> x_i >> y_i >> x_f >> y_f;
            invalid = validate(x_i, y_i, x_f, y_f);
            if (invalid)
                cout << "Invalid move, broken rule #" << invalid << '.' << endl;
        }
        while (invalid);
        move(board[y_i][x_i], board[y_f][x_f]);

        out_board();
        do
        {
            x_i = rand()%8, y_i = rand()%8;
            xy_f = gen_moves(0, tolower(board[y_i][x_i]), x_i, y_i);
            for (const auto& p : xy_f) {
                cout << "(" << p.first << ", " << p.second << "), ";
            }
            cout << endl;
        }
        while (isupper(board[y_i][x_i]) || xy_f.empty());
        move(board[y_i][x_i], board[xy_f[0].second][xy_f[0].first]);

    }
    return 0;
}
