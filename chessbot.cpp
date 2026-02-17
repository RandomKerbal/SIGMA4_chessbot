#include <cmath>
#include <cctype>
#include <vector>
#include <unordered_map>
#include <climits>

// remove in VEXIQ
#include <iostream>
#include <iomanip>

// /**
//  * occupy
//  * ├── 0: black
//  * │   └── coordinates of occupied tiles
//  * │       0: king (since king is always on board)
//  * │       1-8: pawn (since capturing a shape with a pawn is usually good -> enables AlphaBeta prunning)
//  * │
//  * └── 1: white
//  *     └── coordinates of occupied tiles
//  *         0: king (since king is always on board)
//  *         1-8: pawn (since capturing a shape with a pawn is usually good -> enables AlphaBeta prunning)
//  */
// pair<int, int> occupy[2][16] = {
//     { {3, 0},
//       {1, 1}, {2, 1}, {3, 1}, {4, 1}, {5, 1}, {6, 1}, {7, 1},
//       {0, 0}, {1, 0}, {2, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0}, {0, 1} },
//     { {3, 7},
//       {0, 6}, {1, 6}, {2, 6}, {3, 6}, {4, 6}, {5, 6}, {6, 6}, {7, 6},
//       {0, 7}, {1, 7}, {2, 7}, {4, 7}, {5, 7}, {6, 7}, {7, 7} }
// };

// /**
//  * occupy_end
//  * ├── 0: black
//  * │   └── number of shapes remaining on board
//  * │
//  * └── 1: white
//  *     └── number of shapes remaining on board
//  */
// int occupy_end[2] = {16, 16};

/**
 * IMPORTANT: the board is 10x8 since the leftmost and rightmost columns are sentinels (.)
 *            that prevent pieces from "wrapping" onto the previous/next row. The playable
 *            area is the center 8x8.
 * The board is stored as a sparse matrix (never store empty tiles or sentinels) for faster child generation.
 * board
 * ├── key: 1D coordinate of tile (0...99)
 * └── value (aka tile):
 *     ├── lowercase: black, uppercase: white
 *     └── k/K: king, q/Q: queen, r/R: rook, b/B: bishop, n/N: knight, p/P: pawn
 */
std::unordered_map<int, char> board;
const int PLAY_SIZE = 8; // playable size
const int SIZE = PLAY_SIZE + 2; // playable size + sentinels
const int PLAY_AREA = PLAY_SIZE*SIZE;

const int K_VECTOR[8] = {
    +1,             // (1, 0)
    +1 + SIZE,      // (1, 1)
    +SIZE,          // (0, 1)
    -1 + SIZE,      // (-1, 1)
    -1,             // (-1, 0)
    -1 - SIZE,      // (-1, -1)
    -SIZE,          // (0, -1)
    +1 - SIZE       // (1, -1)
};
const int N_VECTOR[8] = {
    2 + SIZE,       // (2, 1)
    1 + 2*SIZE,     // (1, 2)
   -1 + 2*SIZE,     // (-1, 2)
   -2 + SIZE,       // (-2, 1)
   -2 - SIZE,       // (-2, -1)
   -1 - 2*SIZE,     // (-1, -2)
    1 - 2*SIZE,     // (1, -2)
    2 - SIZE        // (2, -1)
};
const int R_VECTOR[4] = {
    +1,             // right
    -1,             // left
    +SIZE,          // down
    -SIZE           // up
};
const int B_VECTOR[4] = {
    +1 + SIZE,      // down-right
    +1 - SIZE,      // up-right
    -1 + SIZE,      // down-left
    -1 - SIZE       // up-left
};
// no Q_VECTOR since it's just a combination of R_VECTOR & B_VECTOR

int MAXER = 1, // white
    MINER = 0; // black

void init_board()
{
    board = {
        // black
        {1, 'r'}, {2, 'n'}, {3, 'b'}, {4, 'q'}, {5, 'k'}, {6, 'b'}, {7, 'n'}, {8, 'r'},
        {11, 'p'}, {12, 'p'}, {13, 'p'}, {14, 'p'}, {15, 'p'}, {16, 'p'}, {17, 'p'}, {18, 'p'},
        // 21..58 empty
        // white
        {61, 'P'}, {62, 'P'}, {63, 'P'}, {64, 'P'}, {65, 'P'}, {66, 'P'}, {67, 'P'}, {68, 'P'},
        {71, 'R'}, {72, 'N'}, {73, 'B'}, {74, 'Q'}, {75, 'K'}, {76, 'B'}, {77, 'N'}, {78, 'R'}
    };
    // create Buckets for max number of shapes to prevent rehash and iterator invalidation
    board.reserve(32);
    board.max_load_factor(INT_MAX);
}

/**
 * IMPORTANT: Must check tile is NOT empty before calling to prevent creating empty tiles!
 * @return the player occupying the given tile.
 * 0: black player, 1: white player
 */
inline bool player_of(char tile)
{
    return bool(isupper(tile)); // casted to bool since isupper can return any integer not just 1
}

/**
 * @return the forward direction relative to the given player.
 * In the configuration where white at bottom and black at top, white's foward = -1, black's foward = +1.
 */
inline int rel_foward(bool player)
{
    return (player) ? -1 : 1;
}

inline int xy_of(int key, bool is_y)
{
    return (is_y) ? key / SIZE : key % SIZE;
}

// inline void move(bool player, int i, int x_i, int y_i, int x_f, int y_f)
// /// TODO: pawn promotion
// {
//     if (board[y_f][x_f] != ' ') // if capture something
//     {
//         // hide element f in occupy by swapping f with the last element and shift last element index forward
//         int &e_occupy_end = occupy_end[!player];
//         swap(occupy[!player][f], occupy[!player][e_occupy_end-1]);
//         e_occupy_end --;
//     }
//     occupy[player][i] = {x_f, y_f};
//     board[y_f][x_f] = board[y_i][x_i];
//     board[y_i][x_i] = ' ';
// }

inline void move(int key_i, int key_f)
/// TODO: pawn promotion
{
    board[key_f] = board[key_i];
    board.erase(key_i);    
}

inline void unmove(int key_i, int key_f, char captured = 0)
{
    board[key_i] = board[key_f];
    if (!captured)
        board.erase(key_f);
    else
        board[key_f] = captured;
}

inline bool is_play_area(int key)
{
    int x = xy_of(key, 0);
    return x != 0 && x != 9 && 0 < key && key < PLAY_AREA;
}

inline bool is_empty(int key)
{
    return board.find(key) == board.end();
}

void out_board()
{
    for (std::pair<int, char> pair : board) {
        std::cout << "[" << pair.first << "] = " << pair.second << "; ";
    }
    std::cout << std::endl;

    std::cout << "   +-------------BOT--------------+" << std::endl;
    for (int key = 0; key < PLAY_AREA; key++)
    {
        if (xy_of(key, 0) == 0) // left sentinel
            std::cout << std::setw(2) << key+1 << " | .";

        else if (xy_of(key, 0) == 9) // right sentinel
            std::cout << "  . | " << key-1 << std::endl;

        else
            std::cout << std::setw(3) << ((is_empty(key)) ? ' ' : board[key]);
    }
    std::cout << "   +-------------YOU--------------+" << std::endl;
}

/**
 * IMPORTANT: Must check tile is NOT empty before calling to prevent creating empty tiles!
 * @return whether the given tile has the enemy of the given player && not king.
 */
inline bool can_capture(bool player, char tile)
{
    return player_of(tile) != player && tolower(tile != 'k');
}

std::vector<int> gen_moves(bool player, int key_i)
{
    std::vector<int> moves;
    int key = 0;
    char shape = tolower(board[key_i]);
    
    if (shape == 'k')
    {
        for (int v: K_VECTOR)
        {
            key = key_i + v;
            if (is_play_area(key) && (is_empty(key) || can_capture(player, board[key]))) //short-circuit at is_empty(key) prevent creating empty tiles
                moves.push_back(key);
        }
    }
    if (shape == 'n')
    {
        for (int v: N_VECTOR)
        {
            key = key_i + v;
            if (is_play_area(key) && (is_empty(key) || can_capture(player, board[key])))
                moves.push_back(key);
        }
    }
    if (shape == 'r' || shape == 'q')
    {
        for (int v: R_VECTOR)
        {
            // travel until on top of a shape
            for (key = key_i + v; is_play_area(key) && is_empty(key); key += v)
                moves.push_back(key);
            
            if (is_play_area(key) && can_capture(player, board[key]))
                moves.push_back(key);

        }
    }
    if (shape == 'b' || shape == 'q')
    {
        for (int v: B_VECTOR)
        {
            for (key = key_i + v; is_play_area(key) && is_empty(key); key += v)
                moves.push_back(key);
            
            if (is_play_area(key) && can_capture(player, board[key]))
                moves.push_back(key);
        }
    }
    if (shape == 'p')
    {
        key_i += rel_foward(player)*SIZE;
        
        /// TODO: remove once added promotion
        if (key_i <= 0 || PLAY_AREA <= key_i)
            return moves;

        // capture moves
        for (int v: {-1, 1})
        {
            key = key_i + v;
            if (!is_empty(key) && can_capture(player, board[key]))
                moves.push_back(key);
        }

        // y-moves
        if (is_empty(key_i))  // y-check done at the top
        {
            moves.push_back(key_i);
        
            if (xy_of(key_i, 1) == 1 || xy_of(key_i, 1) == 6) // if pawn can step 2
            {
                key_i += rel_foward(player)*SIZE;
                if (is_empty(key_i))
                    moves.push_back(key_i);
            }
        }
    }

    return moves;
}

/**
 * @return whether the given tile is currently attacked by the enemy of the given player.
 */
bool is_attacked(bool player, int key_i)
{
    int key = 0;
    /*
    Attacks (except pawn) are reversible. If a shape A' on tile A attacks tile B, then A' on tile B attacks tile A.
    Using this theorem,
    checking if shape A' on A attacks king on K = checking if A' on K attacks on A.
    */
    // Let A' be enemy king.
    for (int v: K_VECTOR)
    {
        key = key_i + v;
        if (!is_empty(key) && player_of(board[key]) != player && tolower(board[key]) == 'k')
            return 1;
    }

    // let A' be enemy knight.
    for (int v: N_VECTOR)
    {
        key = key_i + v;
        if (!is_empty(key) && player_of(board[key]) != player && tolower(board[key]) == 'n')
            return 1;
    }
    // let A' be enemy rook or queen.
    for (int v: R_VECTOR)
    {
        for (key = key_i + v; is_play_area(key) && is_empty(key); key += v)
        {}
        if (!is_empty(key) && player_of(board[key]) != player)
        {
            char shape = tolower(board[key]);
            if (shape == 'r' || shape == 'q')
                return 1;
        }
    }
    // let A' be enemy bishop or queen.
    for (int v: B_VECTOR)
    {
        for (key = key_i + v; is_play_area(key) && is_empty(key); key += v)
        {}
        if (!is_empty(key) && player_of(board[key]) != player)
        {
            char shape = tolower(board[key]);
            if (shape == 'b' || shape == 'q')
                return 1;
        }
    }
    // let A' be enemy pawn
    key_i += rel_foward(player)*SIZE;

    if (0 < key && key < PLAY_AREA)
    {
        for (int v: {-1, 1})
        {
            key = key_i + v;
            if (!is_empty(key) && player_of(board[key]) != player && tolower(board[key]) == 'p')
                return 1;
        }
    }
    return 0;
}

// /**
//  * Minimax + Depth-adjusted Score + AlphaBeta Prunning
//  * User Note: In first call, use depth = 0, alpha = INT_MIN, beta = INT_MAX.
//  */
// int minimax(bool player, int depth, int alpha, int beta)

// {
//     int child_score = 0;

//     for ...
//         capture = board[y_f][x_f]
//         move();
//         if (!is_attacked(player, king))
//             // move exists
//             child_score = minimax(!player, depth+1);
//             if (player == MAXER)
//                 alpha = max(alpha, child_score);
//             else
//                 beta = min(beta, child_score);
            
//             if (beta <= alpha)
//                 break;

//     if (child_score == 0) // if move exists, child_score is highly unlikely to = 0 since minimax() returns a number near +/- infinity
//         if (is_attacked(player, king)) // checkmate
//             return (player == MAXER) ? INT_MIN + depth : INT_MAX - depth;
        
//         // !is_attacked = stalemate
//         return 0;
    
//     // not empty
//     return (player == MAXER) ? alpha : beta;

//     // for (int i = 0; i < occupy_end[player]; i++)
//     // {
//     //     int x_i = occupy[player][i].first, y_i = occupy[player][i].second;

//     //     for (pair<int, int> xy_f: gen_moves(player, x_i, y_i))
//     //     {
//     //         move(board[y_i][x_i], board[xy_f.second][xy_f.first]);

//     //         int kx = occupy[player][0].first, ky = occupy[player][0].second; // king x, king y
//     //         if (is_attacked(player, kx, ky) && )

//     //         else
//     //             recur(!player);
//     //     }
//     // }
// }

/**
 * @return whether move is valid.
 * 0: valid, 1: broken chess rule, 2: empty initial tile, 3: outside play area, 4: friendly fire, 5: blocked path
 * TODO: 5: impersonating bot, 6: checkmate, 7: stalemate
 */
int validate(int key_i, int key_f)
{
    if (is_empty(key_i))
        return 2;

    int player = player_of(board[key_i]), shape = tolower(board[key_i]),
        dx = abs(xy_of(key_f, 0) - xy_of(key_i, 0)), dy = abs(xy_of(key_f, 1) - xy_of(key_i, 1));

    if (!is_play_area(key_f))
        return 3;

    if (!is_empty(key_f) && !can_capture(player, board[key_f]))
        return 4;

    // king && check direction
    if (shape == 'k' && std::max(dx, dy) > 1)
        return 1;

    // queen && check direction
    if (shape == 'q' && dx && dy && dx != dy)
        return 1; 

    // rook && check direction
    if (shape == 'r' && dx && dy)
        return 1;
    
    // bishop && check direction
    if (shape == 'b' && dx != dy)
        return 1;

    // knight && check direction
    if ((shape == 'n') && (dx != 1 || dy != 2) && (dx != 2 || dy != 1))
        return 1;

    // pawn && check direction
    if (shape == 'p')
    {
        if (rel_foward(player) * (key_f - key_i) < 0) // moved backward
            return -1;

        if (is_empty(key_f))
        {
            if (dx != 0)
                return 1;
            
            if (dy > 1 + (xy_of(key_i, 1) == 1 || xy_of(key_i, 1) == 6)) // if pawn at starting line: dy > 1+1; else: dy>1+0
                return 1;
        }
        else if (dx != 1 || dy != 1) // && not empty
            return 1;
    }

    if (shape != 'k' && shape != 'n')
    {
        // check if all tiles on the path are empty
        int v = (key_f - key_i) / std::max(dx, dy); 
        for (int key = key_i + v; key != key_f; key += v)
        {
            if (!is_empty(key))
                return 5;
        }
    }

    // if all valid
    return 0;
}

void console_play()
{
    int key_i = 0, key_f = 0, invalid = 0;
    std::vector<int> keys_f;

    while (true)
    {
        out_board();
        do
        {
            std::cout << "Initial and final tile (key_i key_f): ";
            std::cin >> key_i >> key_f;
            invalid = validate(key_i, key_f);
            if (invalid)
                std::cout << "Invalid move, broken rule #" << invalid << '.' << std::endl;
        }
        while (invalid);
        move(key_i, key_f);

        out_board();
        std::cout << "Tiles attacked: ";
        for (std::pair<int, char> pair : board)
        {
            if (player_of(pair.second) == 0 && is_attacked(0, pair.first))
                std::cout << "{ " << pair.first << ", " << pair.second << " }, ";
        }
        std::cout << std::endl;
        for (std::pair<int, char> pair : board)
        {
            if (player_of(pair.second) == 0)
            {
                keys_f = gen_moves(0, pair.first);
                if (!keys_f.empty())
                {
                    move(pair.first, keys_f[0]);
                    std::cout << "Possible moves from " << pair.first << ": ";
                    for (int key_f : keys_f) {
                        std::cout << key_f << ", ";
                    }
                    std::cout << std::endl;
                    break;
                }
            }
        }
    }
}

int main()
{
    init_board();
    console_play();
    return 0;
}
