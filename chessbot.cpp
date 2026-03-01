#include <vector>
#include <climits>
#include <random>

// remove in VEXIQ
#include <iostream>
#include <iomanip>

std::mt19937_64 rng(67);

enum PLAYER {
    BLACK = 0, WHITE = 1,
    MAX_PLAYER = 2 // max number of players
};

/**
 * IMPORTANT: SHAPE 0 is undefined. It starts at 1 to sync with shapes on board.
 * Start with pieces that capture more commonly (pawns -> knights -> bishops -> rooks -> queens -> king)
 * to mimic human behavior and enable more AlphaBeta Prunning.
 */
enum SHAPE {
    PAWN = 1, KNIGHT = 2, BISHOP = 3, ROOK = 4, QUEEN = 5, KING = 6,
    MAX_SHAPE = 7 // max number of shapes +1
};

/**
 * NUM
 * └── 1...6: see enum SHAPE
 *     └── number of pieces of each shape in main board. Starting number of black pieces must = white pieces.
 */
int NUM[MAX_SHAPE] = {0};

/**
 * WORTH
 * └── 1...6: see enum SHAPE
 *     └── points worth of each shape
 */
const int WORTH[MAX_SHAPE] = { 0, 1, 3, 3, 5, 9, 0 };
int MAX_WORTH_NOPAWN = 0; // sum of number of pieces of each shape except pawns * points worth of each shape
const int HEIGHT = 8, WIDTH = HEIGHT + 2, AREA = HEIGHT*WIDTH;

/** 
 * board
 * ├── columns 8,9 element
 * │   └── 0: sentinel
 * │
 * └── columns 0...8 element (tile)
 *     └── 0: empty
 *     ├── digit1: see enum PLAYER
 *     ├── digit2: see enum SHAPE
 *     └── digit3 (indB2): index on board2
 * 
 * The board is 10x8 since the two rightmost columns are sentinels that prevent
 * pointers from "wrapping" onto the previous/next row. The real playable area
 * is the leftmost 8x8. Sentinel/empty are 0 to identify easily using !board[ind]
 */
int board[AREA] = {
     40,  20,  30,  50,  60,  31,  21,  41, 0, 0,
     10,  11,  12,  13,  14,  15,  16,  17, 0, 0,
      0,   0,   0,   0,   0,   0,   0,   0, 0, 0,
      0,   0,   0,   0,   0,   0,   0,   0, 0, 0,
      0,   0,   0,   0,   0,   0,   0,   0, 0, 0,
      0,   0,   0,   0,   0,   0,   0,   0, 0, 0,
    110, 111, 112, 113, 114, 115, 116, 117, 0, 0,
    140, 120, 130, 150, 160, 131, 121, 141, 0, 0,
};

/**
 * board2 (aka B2)
 * └── 0,1: see enum PLAYER
 *     └── 1...6: see enum SHAPE
 *         └── 0...NUM[SHAPE]-1
 *             └── indexes on main board. If a tile is captured, its index becomes: index - AREA.
 * 
 * board2 advantages:
 * 1. Do not need to iterate over empty/enemy tiles in minimax().
 * 2. Quick access of enemy tiles index in is_attacked().
 */
int board2[MAX_PLAYER][MAX_SHAPE][8] = {{{0}}};

/**
 * hash of board
 */
unsigned long long int hash;

/**
 * Ztable (Zobrist Table)
 * └── 0...80: index on main board
 *     └── 0,1: see enum PLAYER
 *         └── 1...6: see enum SHAPE
 *             └── random 64-bit integer
 * 
 * Zplayer: random 64-bit integer
 */
unsigned long long int Ztable[AREA][MAX_PLAYER][MAX_SHAPE] = {{{0}}};
unsigned long long int Zplayer = 0;

/**
 * Transposition table.
 * index = hash % TTABLE_SZ; value = { hash, score }
 * 
 * 2 or more hash may have the same index.
 */
const int TTABLE_SZ = std::pow(2, 21);
std::vector<std::pair<unsigned long long int, int>> Ttable(TTABLE_SZ, {0, 0});

const int K_VECTOR[8] = {
    +1,             // (1, 0)
    +1 + WIDTH,     // (1, 1)
    +WIDTH,         // (0, 1)
    -1 + WIDTH,     // (-1, 1)
    -1,             // (-1, 0)
    -1 - WIDTH,     // (-1, -1)
    -WIDTH,         // (0, -1)
    +1 - WIDTH      // (1, -1)
};
const int N_VECTOR[8] = {
    2 + WIDTH,      // (2, 1)
    1 + 2*WIDTH,    // (1, 2)
   -1 + 2*WIDTH,    // (-1, 2)
   -2 + WIDTH,      // (-2, 1)
   -2 - WIDTH,      // (-2, -1)
   -1 - 2*WIDTH,    // (-1, -2)
    1 - 2*WIDTH,    // (1, -2)
    2 - WIDTH       // (2, -1)
};
const int B_VECTOR[4] = {
    +1 + WIDTH,     // down-right
    +1 - WIDTH,     // up-right
    -1 + WIDTH,     // down-left
    -1 - WIDTH      // up-left
};
const int R_VECTOR[4] = {
    +1,             // right
    -1,             // left
    +WIDTH,         // down
    -WIDTH          // up
};
// no Q_VECTOR since it's a combination of B_VECTOR & R_VECTOR

/**
 * MIDGAME/ENDGAME_BONUS
 * └── 1...6: see enum SHAPE
 *     └── 0...AREA-1: see main board
 *         └── bonus piece value based on position during midgame/endgame with player at the bottom
 * 
 * values from Rofchade: http://www.talkchess.com/forum3/viewtopic.php?f=2&t=68311&start=19
 */
const int MIDGAME_BONUS[MAX_SHAPE][AREA] = {
    {0},
    // pawn
    {
          0,   0,   0,   0,   0,   0,  0,   0, 0, 0,
         98, 134,  61,  95,  68, 126, 34, -11, 0, 0,
         -6,   7,  26,  31,  65,  56, 25, -20, 0, 0,
        -14,  13,   6,  21,  23,  12, 17, -23, 0, 0,
        -27,  -2,  -5,  12,  17,   6, 10, -25, 0, 0,
        -26,  -4,  -4, -10,   3,   3, 33, -12, 0, 0,
        -35,  -1, -20, -23, -15,  24, 38, -22, 0, 0,
          0,   0,   0,   0,   0,   0,  0,   0, 0, 0
    },
    // knight
    {
        -167, -89, -34, -49,  61, -97, -15, -107, 0, 0,
         -73, -41,  72,  36,  23,  62,   7,  -17, 0, 0,
         -47,  60,  37,  65,  84, 129,  73,   44, 0, 0,
          -9,  17,  19,  53,  37,  69,  18,   22, 0, 0,
         -13,   4,  16,  13,  28,  19,  21,   -8, 0, 0,
         -23,  -9,  12,  10,  19,  17,  25,  -16, 0, 0,
         -29, -53, -12,  -3,  -1,  18, -14,  -19, 0, 0,
        -105, -21, -58, -33, -17, -28, -19,  -23, 0, 0
    },
    // bishop
    {
        -29,   4, -82, -37, -25, -42,   7,  -8, 0, 0,
        -26,  16, -18, -13,  30,  59,  18, -47, 0, 0,
        -16,  37,  43,  40,  35,  50,  37,  -2, 0, 0,
         -4,   5,  19,  50,  37,  37,   7,  -2, 0, 0,
         -6,  13,  13,  26,  34,  12,  10,   4, 0, 0,
          0,  15,  15,  15,  14,  27,  18,  10, 0, 0,
          4,  15,  16,   0,   7,  21,  33,   1, 0, 0,
        -33,  -3, -14, -21, -13, -12, -39, -21, 0, 0
    },
    // rook
    {
         32,  42,  32,  51, 63,  9,  31,  43, 0, 0,
         27,  32,  58,  62, 80, 67,  26,  44, 0, 0,
         -5,  19,  26,  36, 17, 45,  61,  16, 0, 0,
        -24, -11,   7,  26, 24, 35,  -8, -20, 0, 0,
        -36, -26, -12,  -1,  9, -7,   6, -23, 0, 0,
        -45, -25, -16, -17,  3,  0,  -5, -33, 0, 0,
        -44, -16, -20,  -9, -1, 11,  -6, -71, 0, 0,
        -19, -13,   1,  17, 16,  7, -37, -26, 0, 0
    },
    // queen
    {
        -28,   0,  29,  12,  59,  44,  43,  45, 0, 0,
        -24, -39,  -5,   1, -16,  57,  28,  54, 0, 0,
        -13, -17,   7,   8,  29,  56,  47,  57, 0, 0,
        -27, -27, -16, -16,  -1,  17,  -2,   1, 0, 0,
         -9, -26,  -9, -10,  -2,  -4,   3,  -3, 0, 0,
        -14,   2, -11,  -2,  -5,   2,  14,   5, 0, 0,
        -35,  -8,  11,   2,   8,  15,  -3,   1, 0, 0,
         -1, -18,  -9,  10, -15, -25, -31, -50, 0, 0
    },
    // king
    {
        -65,  23,  16, -15, -56, -34,   2,  13, 0, 0,
         29,  -1, -20,  -7,  -8,  -4, -38, -29, 0, 0,
         -9,  24,   2, -16, -20,   6,  22, -22, 0, 0,
        -17, -20, -12, -27, -30, -25, -14, -36, 0, 0,
        -49,  -1, -27, -39, -46, -44, -33, -51, 0, 0,
        -14, -14, -22, -46, -44, -30, -15, -27, 0, 0,
          1,   7,  -8, -64, -43, -16,   9,   8, 0, 0,
        -15,  36,  12, -54,   8, -28,  24,  14, 0, 0
    }
};
const int ENDGAME_BONUS[MAX_SHAPE][AREA] = {
    {0},
    // pawn
    {
          0,   0,   0,   0,   0,   0,   0,   0, 0, 0,
        178, 173, 158, 134, 147, 132, 165, 187, 0, 0,
         94, 100,  85,  67,  56,  53,  82,  84, 0, 0,
         32,  24,  13,   5,  -2,   4,  17,  17, 0, 0,
         13,   9,  -3,  -7,  -7,  -8,   3,  -1, 0, 0,
          4,   7,  -6,   1,   0,  -5,  -1,  -8, 0, 0,
         13,   8,   8,  10,  13,   0,   2,  -7, 0, 0,
          0,   0,   0,   0,   0,   0,   0,   0, 0, 0
    },
    // knight
    {
        -58, -38, -13, -28, -31, -27, -63, -99, 0, 0,
        -25,  -8, -25,  -2,  -9, -25, -24, -52, 0, 0,
        -24, -20,  10,   9,  -1,  -9, -19, -41, 0, 0,
        -17,   3,  22,  22,  22,  11,   8, -18, 0, 0,
        -18,  -6,  16,  25,  16,  17,   4, -18, 0, 0,
        -23,  -3,  -1,  15,  10,  -3, -20, -22, 0, 0,
        -42, -20, -10,  -5,  -2, -20, -23, -44, 0, 0,
        -29, -51, -23, -15, -22, -18, -50, -64, 0, 0
    },
    // bishop
    {
        -14, -21, -11,  -8, -7,  -9, -17, -24, 0, 0,
         -8,  -4,   7, -12, -3, -13,  -4, -14, 0, 0,
          2,  -8,   0,  -1, -2,   6,   0,   4, 0, 0,
         -3,   9,  12,   9, 14,  10,   3,   2, 0, 0,
         -6,   3,  13,  19,  7,  10,  -3,  -9, 0, 0,
        -12,  -3,   8,  10, 13,   3,  -7, -15, 0, 0,
        -14, -18,  -7,  -1,  4,  -9, -15, -27, 0, 0,
        -23,  -9, -23,  -5, -9, -16,  -5, -17, 0, 0
    },
    // rook
    {
        13, 10, 18, 15, 12,  12,   8,   5, 0, 0,
        11, 13, 13, 11, -3,   3,   8,   3, 0, 0,
         7,  7,  7,  5,  4,  -3,  -5,  -3, 0, 0,
         4,  3, 13,  1,  2,   1,  -1,   2, 0, 0,
         3,  5,  8,  4, -5,  -6,  -8, -11, 0, 0,
        -4,  0, -5, -1, -7, -12,  -8, -16, 0, 0,
        -6, -6,  0,  2, -9,  -9, -11,  -3, 0, 0,
        -9,  2,  3, -1, -5, -13,   4, -20, 0, 0
    },
    // queen
    {
         -9,  22,  22,  27,  27,  19,  10,  20, 0, 0,
        -17,  20,  32,  41,  58,  25,  30,   0, 0, 0,
        -20,   6,   9,  49,  47,  35,  19,   9, 0, 0,
          3,  22,  24,  45,  57,  40,  57,  36, 0, 0,
        -18,  28,  19,  47,  31,  34,  39,  23, 0, 0,
        -16, -27,  15,   6,   9,  17,  10,   5, 0, 0,
        -22, -23, -30, -16, -16, -23, -36, -32, 0, 0,
        -33, -28, -22, -43,  -5, -32, -20, -41, 0, 0
    },
    // king
    {
        -74, -35, -18, -18, -11,  15,   4, -17, 0, 0,
        -12,  17,  14,  17,  17,  38,  23,  11, 0, 0,
         10,  17,  23,  15,  20,  45,  44,  13, 0, 0,
         -8,  22,  24,  27,  26,  33,  26,   3, 0, 0,
        -18,  -4,  21,  24,  27,  23,   9, -11, 0, 0,
        -19,  -3,  11,  21,  23,  16,   7,  -9, 0, 0,
        -27, -11,   4,  13,  14,   4,  -5, -17, 0, 0,
        -53, -34, -21, -11, -28, -14, -24, -43, 0, 0
    }
};

/**
 * midgame/endgame_val (aka piece-square table)
 * └── 0,1: see enum PLAYER
 *     └── 1...6: see enum SHAPE
 *         └── 0...AREA-1: see main board
 *             └── MIDGAME/ENDGAME_BASE + MIDGAME/ENDGAME_BONUS
 */
int midgame_val[MAX_PLAYER][MAX_SHAPE][AREA] = {{{0}}};
int endgame_val[MAX_PLAYER][MAX_SHAPE][AREA] = {{{0}}};

const bool MAXER = WHITE, MINER = BLACK;
const int MAX_DEPTH = 6;

/**
 * char_of
 * ├── 0 (black)
 * │   └── 1...6: see enum SHAPE
 * │
 * └── 1 (white)
 *     └── 1...6: see enum SHAPE
 * 
 * Converts tile from integer to character.
 * lowercase: black; uppercase: white 
 */
const char char_of[MAX_PLAYER][MAX_SHAPE] = {
    { ' ', 'p', 'n', 'b', 'r', 'q', 'k' },
    { ' ', 'P', 'N', 'B', 'R', 'Q', 'K' },
};

/**
 * IMPORTANT: Before calling, use board[ind] to check tile is NOT empty since return 0 can = black or empty!
 * @return the player occupying the given tile (see enum PLAYER).
 */
inline bool player_of(int tile)
{
    return tile / 100; // extract digit1
}

inline int shape_of(int tile)
{
    return (tile / 10) % 10; // extract digit2
}

inline int indB2_of(int tile)
{
    return tile % 10; // extract digit3
}

inline int x_of(int ind)
{
    return ind % WIDTH;
}

inline int y_of(int ind)
{
    return ind / WIDTH;
}

/**
 * @return the forward direction relative to the given player.
 * With WHITE at the bottom and black at the top, white's foward = -WIDTH, black's foward = +WIDTH.
 */
inline int rel_foward(bool player)
{
    return (player ? -1 : 1)*WIDTH;
}

inline bool is_play_area(int ind)
{
    return x_of(ind) < 8 && unsigned(ind) < AREA; // if x < 0, it becomes a huge unsigned number > AREA.
}

/**
 * Initialize board2, NUM, MAX_WORTH, Ztable, hash, midgame_val, and endgame_val.
 */
void init_all()
{
    // board2, NUM
    for (int ind = 0; ind < AREA; ind++)
    {
        int tile = board[ind];
        if (tile)
        {
            bool player = player_of(tile);
            int shape = shape_of(tile);
            board2[player][shape][indB2_of(tile)] = ind;

            if (player)  // only count num of white pieces
                NUM[shape]++;
        }
    }

    // MAX_WORTH
    for (int shape = KNIGHT; shape < KING; shape++)
    {
        MAX_WORTH_NOPAWN += WORTH[shape]*NUM[shape];
    }

    // Zplayer, Ztable
    Zplayer = rng();
    for (int ind = 0; ind < AREA; ind++)
    {
        for (int player = BLACK; player <= WHITE; player++)
        {
            for (int shape = PAWN; shape < MAX_SHAPE; shape++)
                Ztable[ind][player][shape] = rng();
        }
    }

    // hash
    for (int player = BLACK; player <= WHITE; player++)
    {
        for (int shape = PAWN; shape < MAX_SHAPE; shape++)
        {
            for (int indB2 = 0; indB2 < NUM[shape]; indB2++)
            {
                int ind = board2[player][shape][indB2];
                hash ^= Ztable[ind][player][shape];
            }
        }
    }

    // midgame_val, endgame_val
    for (int player = BLACK; player <= WHITE; player++)
    {
        bool flip = rel_foward(player) > 0; // flip if player at the top
        for (int shape = PAWN; shape < MAX_SHAPE; shape++)
        {
            for (int ind = 0; ind < AREA; ind++)
            {
                if (is_play_area(ind))
                {
                    int ind_bonus = (flip) ? (HEIGHT-1 - y_of(ind)) * WIDTH + x_of(ind) : ind;

                    midgame_val[player][shape][ind] = WORTH[shape]*100 + MIDGAME_BONUS[shape][ind_bonus];
                    endgame_val[player][shape][ind] = WORTH[shape]*100 + ENDGAME_BONUS[shape][ind_bonus];
                }

            }
        }
    }
}

/**
 * @return captured tile.
 * When undoing moves,
 * 1. returned value is directly fed into parameter: restore.
 * 2. ind_i and ind_f switch places.
 */
inline int move(bool player, int shape, int indB2, int ind_i, int ind_f, int restore = 0)
{
    int capture = board[ind_f];
    if (capture)
    {
        int shape_capture = shape_of(capture);
        board2[!player][shape_capture][indB2_of(capture)] -= AREA; // move ind_f outside board
        hash ^= Ztable[ind_f][!player][shape_capture];
    }

    else if (restore) // if tile to restore exists
    {
        int shape_restore = shape_of(restore);
        board2[!player][shape_restore][indB2_of(restore)] += AREA; // move ind_f outside board back inside
        hash ^= Ztable[ind_i][!player][shape_restore];
    }

    board2[player][shape][indB2] = ind_f;

    // add to final tile
    board[ind_f] = board[ind_i];
    hash ^= Ztable[ind_f][player][shape];
    
    // remove from initial tile
    board[ind_i] = restore;
    hash ^= Ztable[ind_i][player][shape];

    hash ^= Zplayer;  // switch player of hash
    return capture;
}

/**
 * IMPORTANT: Before calling, use board[ind] to check tile is NOT empty since return 0 can = black or empty!
 * @return whether the given tile has the enemy of the given player && not king.
 */
inline bool can_capture(bool player, int tile)
{
    return player_of(tile) != player && shape_of(tile) != KING;
}

std::vector<int> gen_moves(bool player, int shape, int ind_i)
{
    std::vector<int> moves;
    moves.reserve(27);
    int ind = 0, tile = 0;
    
    if (shape == KING)
    {
        for (int v: K_VECTOR)
        {
            ind = ind_i + v;
            tile = board[ind];
            if (is_play_area(ind) && (!tile || can_capture(player, tile)))
                moves.emplace_back(ind);
        }
    }
    if (shape == KNIGHT)
    {
        for (int v: N_VECTOR)
        {
            ind = ind_i + v;
            tile = board[ind];
            if (is_play_area(ind) && (!tile || can_capture(player, tile)))
                moves.emplace_back(ind);
        }
    }
    if (shape == ROOK || shape == QUEEN)
    {
        for (int v: R_VECTOR)
        {
            // travel until on top of a shape
            for (ind = ind_i + v; is_play_area(ind) && !board[ind]; ind += v)
                moves.emplace_back(ind);
            
            if (is_play_area(ind) && can_capture(player, board[ind]))
                moves.emplace_back(ind);

        }
    }
    if (shape == BISHOP || shape == QUEEN)
    {
        for (int v: B_VECTOR)
        {
            for (ind = ind_i + v; is_play_area(ind) && !board[ind]; ind += v)
                moves.emplace_back(ind);
            
            if (is_play_area(ind) && can_capture(player, board[ind]))
                moves.emplace_back(ind);
        }
    }
    if (shape == PAWN)
    {
        int y_i = y_of(ind_i), dy = rel_foward(player);
        if (1 <= y_i && y_i <= 6)
        {
            ind = ind_i + dy;

            // capture moves
            for (int dx = -1; dx <= 1; dx = dx + 2)
            {
                int ind_f = ind + dx;
                tile = board[ind_f];
                if (tile && can_capture(player, tile))
                    moves.emplace_back(ind_f);
            }

            // y-moves
            if (!board[ind])
            {
                moves.emplace_back(ind);
            
                if ((y_i == 1 && dy > 0) || (y_i == 6 && dy < 0)) // if pawn can step 2
                {
                    ind += dy;
                    if (!board[ind])
                        moves.emplace_back(ind);
                }
            }
        }
    }

    return moves;
}

/**
 * @return whether all tiles on the path are empty.
 * IMPORTANT: dx, dy are absolute.
 */
inline bool is_path_clear(int ind_i, int ind_f, int dx, int dy)
{
    int v = (ind_f - ind_i) / std::max(dx, dy); 
    for (int ind = ind_i + v; ind != ind_f; ind += v)
    {
        if (board[ind])
            return false;
    }
    return true;
}

/**
 * @return whether the given tile is currently attacked by the enemy of the given player.
 */
bool is_attacked(bool player, int ind)
{
    int ind_e = 0, // enemy index on main board
        dx = 0, dy = 0;

    // Start with pieces more likely to be attacked by (pawns -> knights -> bishops -> rooks -> queens -> king).

    // check for enemy pawns
    // Pretend "I" am a pawn and check where "I" can capture.
    const int PAWN_e = !player*10 + PAWN;
    ind_e = ind + rel_foward(player);
    if ((is_play_area(ind_e + 1) && board[ind_e + 1] == PAWN_e) ||
        (is_play_area(ind_e - 1) && board[ind_e - 1] == PAWN_e))
        return 1;

    // check for enemy knights
    // Iterate over all enemy knights.
    for (int indB2_e = 0; indB2_e < NUM[KNIGHT]; indB2_e++)
    {
        ind_e = board2[!player][KNIGHT][indB2_e];
        if (ind_e >= 0) // if not captured
        {
            dx = abs(x_of(ind_e) - x_of(ind));
            dy = abs(y_of(ind_e) - y_of(ind));
            if ((dx == 1 && dy == 2) && (dx == 2 && dy == 1))
                return 1;
        }
    }

    // check for enemy bishops
    for (int indB2_e = 0; indB2_e < NUM[BISHOP]; indB2_e++)
    {
        ind_e = board2[!player][BISHOP][indB2_e];
        if (ind_e >= 0)
        {
            dx = abs(x_of(ind_e) - x_of(ind));
            dy = abs(y_of(ind_e) - y_of(ind));
            if (dx == dy && is_path_clear(ind, ind_e, dx, dy))
                return 1;
        }
    }

    // check for enemy rooks
    for (int indB2_e = 0; indB2_e < NUM[ROOK]; indB2_e++)
    {
        ind_e = board2[!player][ROOK][indB2_e];
        if (ind_e >= 0)
        {
            dx = abs(x_of(ind_e) - x_of(ind));
            dy = abs(y_of(ind_e) - y_of(ind));
            if ((dx == 0 || dy == 0) && is_path_clear(ind, ind_e, dx, dy))
                return 1;
        }
    }

    // check for enemy queen
    ind_e = board2[!player][QUEEN][0];
    if (ind_e >= 0)
    {
        dx = abs(x_of(ind_e) - x_of(ind));
        dy = abs(y_of(ind_e) - y_of(ind));
        if ((dx == dy || dx == 0 || dy == 0) && is_path_clear(ind, ind_e, dx, dy))
            return 1;
    }

    // check for enemy king
    ind_e = board2[!player][KING][0];
    dx = abs(x_of(ind_e) - x_of(ind));
    dy = abs(y_of(ind_e) - y_of(ind));
    if (std::max(dx, dy) == 1)
        return 1;

    return 0;
}

void out_board(bool has_Ttable = false, bool has_hash = false, bool has_board2 = false, bool has_attack = false)
{
    if (has_Ttable)
    {
        std::cout << "Ttable:" << std::endl;
        int i = 0;
        for (std::pair<unsigned long long int, int> hashscore : Ttable)
        {
            if (i < 10 && hashscore.first)
            {
                i++;
                std::cout << "  " << hashscore.first << ": " << hashscore.second << '\n';
            }
            else
            {
                std::cout << "  ..." << std::endl;
                break;
            }
        }
        std::cout << "Ttable size: " << Ttable.size() << std::endl;
    }

    if (has_hash)
        std::cout << "Hash:" << std::endl << "  " << hash << std::endl;

    if (has_board2)
    {
        std::cout << "Board2:" << std::endl;
        for (int player = BLACK; player <= WHITE; player++)
        {
            for (int shape = PAWN; shape < MAX_SHAPE; shape++)
            {
                std::cout << std::setw(3) << '[' << char_of[player][shape] << "]=";
                for (int indB2 = 0; indB2 < NUM[shape]; indB2++)
                {
                    int ind = board2[player][shape][indB2];
                    std::cout << ind << ",";
                }
            }
            std::cout << std::endl;
        }
    }

    if (has_attack)
    {
        for (bool player : { BLACK, WHITE })
        {
            std::cout << "Player" << player << " attacks: " << std::endl << "  ";
            for (int ind = 0; ind < AREA; ind++)
            {
                int tile = board[ind];
                if (tile && player_of(tile) != player && is_attacked(!player, ind))
                    std::cout << ind << ", ";
            }
            std::cout << std::endl;
        }
    }

    std::cout << "   +-------------BOT-------------+" << std::endl;
    for (int ind = 0; ind < AREA; ind++)
    {
        if (x_of(ind) > 7) // sentinels
        {
            std::cout << " . . | " << ind - 1 << std::endl;
            ind++;
        }
        else
        {
            if (x_of(ind) == 0)
                std::cout << std::setw(2) << ind << " |";

            int tile = board[ind];
            std::cout << std::setw(3) << char_of[player_of(tile)][shape_of(tile)];
        }
    }
    std::cout << "   +-------------YOU-------------+" << std::endl;
}

int approx(bool player)
{
    int midgame[2] = {0, 0}, endgame[2] = {0, 0};
    float worths = 0; // sum of worth of remaining pieces

    for (int player = BLACK; player <= WHITE; player++)
    {
        for (int shape = KNIGHT; shape < KING; shape++) // start from knight since pawn is insignificant
        {
            for (int indB2 = 0; indB2 < NUM[shape]; indB2++)
            {
                int ind = board2[player][shape][indB2];
                if (ind >= 0) // if not captured
                {
                    midgame[player] += midgame_val[player][shape][ind];
                    endgame[player] += endgame_val[player][shape][ind];
                    worths += WORTH[shape];
                }
            }
        }
    }

    int midgame_score = midgame[player] - midgame[!player],
        endgame_score = endgame[player] - endgame[!player];

    return endgame_score - (worths/MAX_WORTH_NOPAWN)*(endgame_score - midgame_score); // linear interpolation
}

/**
 * Minimax + Tapered Piece-Square Table Evaluation + AlphaBeta Prunning + Zobrist Hashed Transposition Table
 * In first call, use depth = 1, alpha = INT_MIN, beta = INT_MAX.
 * @return
 *      n ∈ [-MAX_WORTH_NOPAWN, MAX_WORTH_NOPAWN] if over MAX_DEPTH || draw.
 *      n = INT_MAX if checked by MAXER.
 *      n = INT_MIN if checked by MINER.
 */
int minimax(bool player, int depth, int alpha, int beta)
{
    std::pair<unsigned long long int, int> &ind_Ttable = Ttable[hash % TTABLE_SZ];
    if (ind_Ttable.first == hash)
        return ind_Ttable.second;

    if (depth > MAX_DEPTH)
        return approx(player) * (player == MAXER ? 1 : -1);

    int capture = 0, score = 0, child_score = 0;
    bool is_checked = false;

    if (is_attacked(player, board2[player][KING][0]))
        is_checked = true;
    
    for (int shape = PAWN; shape < MAX_SHAPE; shape++)
    {
        for (int indB2 = 0; indB2 < NUM[shape]; indB2++)
        {
            int ind_i = board2[player][shape][indB2];
            if (ind_i >= 0) // if not captured
            {
                for (int ind_f : gen_moves(player, shape, ind_i))
                {
                    capture = move(player, shape, indB2, ind_i, ind_f);
                    if (is_checked)
                    {
                        if (is_attacked(player, board2[player][KING][0]))
                        {
                            move(player, shape, indB2, ind_f, ind_i, capture); // undo move
                            continue;
                        }
                        is_checked = false;
                    }
                    
                    child_score = minimax(!player, depth+1, alpha, beta);

                    if (player == MAXER)
                        alpha = std::max(alpha, child_score);
                    else
                        beta = std::min(beta, child_score);

                    if (beta <= alpha)
                    {
                        move(player, shape, indB2, ind_f, ind_i, capture); // undo move
                        score = (player == MAXER) ? alpha : beta;
                        ind_Ttable.second = score;
                        return score;
                    }
                    move(player, shape, indB2, ind_f, ind_i, capture); // undo move
                }
            }
        }
    }

    if (is_checked) // checkmate
        score = (player == MAXER) ? INT_MIN : INT_MAX;
    else
        score = (player == MAXER) ? alpha : beta;
    ind_Ttable.second = score;
    return score;
}

/**
 * Behave the same as minimax(), except with recording the best move and without recording to Ttable and AlphaBeta Prunning.
 */
void bot_move(bool player)
{
    int capture = 0, child_score = 0, best_indB2 = 0, best_shape = 0, best_ind_i = 0, best_ind_f = 0, best_score = (player == MAXER) ? INT_MIN : INT_MAX;

    for (int shape = PAWN; shape < MAX_SHAPE; shape++)
    {
        for (int indB2 = 0; indB2 < NUM[shape]; indB2++)
        {
            int ind_i = board2[player][shape][indB2];
            if (ind_i >= 0) // if not captured
            {
                std::cout << "Possible {move, score} from " << ind_i << ": ";
                for (int ind_f : gen_moves(player, shape, ind_i))
                {
                    capture = move(player, shape, indB2, ind_i, ind_f),
                    
                    child_score = minimax(!player, 1, INT_MIN, INT_MAX);
                    
                    std::cout << "{" << ind_f << ", " << child_score << "}, ";
                    if (player == MAXER)
                    {
                        if (child_score > best_score)
                        {
                            best_indB2 = indB2;
                            best_shape = shape;
                            best_ind_i = ind_i;
                            best_ind_f = ind_f;
                            best_score = child_score;
                        }
                    }
                    else if (child_score < best_score) // if MINER
                    {
                        best_indB2 = indB2;
                        best_shape = shape;
                        best_ind_i = ind_i;
                        best_ind_f = ind_f;
                        best_score = child_score;
                    }
                    move(player, shape, indB2, ind_f, ind_i, capture); // undo move
                }
                std::cout << std::endl;
            }
        }
    }
    std::cout << "Chosen move: " << best_ind_i << " to " << best_ind_f << std::endl;
    move(player, best_shape, best_indB2, best_ind_i, best_ind_f);
}

/**
 * @return whether move is valid.
 * 0: valid, 1: broken chess rule, 2: empty initial tile, 3: outside play area, 4: friendly fire, 5: blocked path
 * TODO: 5: impersonating bot, 6: checkmate, 7: draw
 */
int validate(int ind_i, int ind_f)
{
    bool player = player_of(board[ind_i]);
    int shape = shape_of(board[ind_i]),
        dx = abs(x_of(ind_f) - x_of(ind_i)),
        dy = abs(y_of(ind_f) - y_of(ind_i));

    if (!board[ind_i])
        return 2;

    if (!is_play_area(ind_f))
        return 3;

    if (board[ind_f] && !can_capture(player, board[ind_f]))
        return 4;

    // king && check direction
    if (shape == KING && std::max(dx, dy) > 1)
        return 1;

    // knight && check direction
    if ((shape == KNIGHT) && (dx != 1 || dy != 2) && (dx != 2 || dy != 1))
        return 1;

    // queen && check direction
    if (shape == QUEEN && dx && dy && dx != dy)
        return 1; 

    // rook && check direction
    if (shape == ROOK && dx && dy)
        return 1;
    
    // bishop && check direction
    if (shape == BISHOP && dx != dy)
        return 1;

    // pawn && check direction
    if (shape == PAWN)
    {
        if (rel_foward(player)*(ind_f - ind_i) < 0) // moved backward
            return 1;

        if (!board[ind_f])
        {
            if (dx != 0)
                return 1;
            
            if (dy > 1 + (y_of(ind_i) == 1 || y_of(ind_i) == 6)) // if pawn at starting line: dy > 1+(1); else: dy>1+(0)
                return 1;
        }
        else if (dx != 1 || dy != 1) // && not empty
            return 1;
    }

    if (shape != KING && shape != KNIGHT && !is_path_clear(ind_i, ind_f, dx, dy))
        return 5;

    // if all valid
    return 0;
}

void console_play()
{
    int ind_i = 0, ind_f = 0, invalid = 0;
    std::vector<int> inds_f;

    while (true)
    {
        out_board(true, true, true, true);
        do
        {
            std::cout << "Initial and final tile (ind_i ind_f): ";
            std::cin >> ind_i >> ind_f;
            invalid = validate(ind_i, ind_f);
            if (invalid)
                std::cout << "Invalid move, broken rule #" << invalid << std::endl;
        }
        while (invalid);
        move(WHITE, shape_of(board[ind_i]), indB2_of(board[ind_i]), ind_i, ind_f);
        std::cout << std::endl;

        bot_move(BLACK);
        std::cout << std::endl;
    }
}

int main()
{
    // IMPORTANT: Must call init_all() at the start of every program!
    init_all();
    console_play();
    return 0;
}
