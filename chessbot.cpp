#include <vector>
#include <climits>
#include <random>

// remove in VEXIQ
#include <iostream>
#include <iomanip>

std::mt19937_64 rng(0);

enum PLAYER {
    BLACK = 0, MINER = BLACK, WHITE = 1, MAXER = WHITE,
    MAX_PLAYER = 2 // max number of players
};
const short MAX_DEPTH = 5;

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
 *     └── number of pieces of each shape in main board. If starting number of black pieces != white pieces, the higher number.
 */
short NUM[MAX_SHAPE] = {0};
const short HEIGHT = 8, WIDTH = HEIGHT + 2, AREA = HEIGHT*WIDTH;

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
short board[AREA] = {
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
short board2[MAX_PLAYER][MAX_SHAPE][8] = {{{-1}}};

/**
 * hash of board
 */
unsigned long long hash;

/**
 * Ztable (Zobrist Table)
 * └── 0...80: index on main board
 *     └── 0,1: see enum PLAYER
 *         └── 1...6: see enum SHAPE
 *             └── random 64-bit integer
 * 
 * Zplayer: random 64-bit integer
 */
unsigned long long Ztable[AREA][MAX_PLAYER][MAX_SHAPE] = {{{0}}};
unsigned long long Zplayer = 0;

/**
 * Transposition table.
 * index = hash % TTABLE_SZ; value = { hash, score }
 * 
 * 2 or more hash may have the same index.
 */
const unsigned int TTABLE_SZ = std::pow(2, 22);
struct Ttable_entry {
    unsigned long long hash = 0;
    short score = 0;
    short depth = 0;
};
std::vector<Ttable_entry> Ttable(TTABLE_SZ, {0, 0, 0});

const short K_VECTOR[8] = {
    +1,             // (1, 0)
    +1 + WIDTH,     // (1, 1)
    +WIDTH,         // (0, 1)
    -1 + WIDTH,     // (-1, 1)
    -1,             // (-1, 0)
    -1 - WIDTH,     // (-1, -1)
    -WIDTH,         // (0, -1)
    +1 - WIDTH      // (1, -1)
};
const short N_VECTOR[8] = {
    2 + WIDTH,      // (2, 1)
    1 + 2*WIDTH,    // (1, 2)
   -1 + 2*WIDTH,    // (-1, 2)
   -2 + WIDTH,      // (-2, 1)
   -2 - WIDTH,      // (-2, -1)
   -1 - 2*WIDTH,    // (-1, -2)
    1 - 2*WIDTH,    // (1, -2)
    2 - WIDTH       // (2, -1)
};
const short B_VECTOR[4] = {
    +1 + WIDTH,     // down-right
    +1 - WIDTH,     // up-right
    -1 + WIDTH,     // down-left
    -1 - WIDTH      // up-left
};
const short R_VECTOR[4] = {
    +1,             // right
    -1,             // left
    +WIDTH,         // down
    -WIDTH          // up
};
// no Q_VECTOR since it's a combination of B_VECTOR & R_VECTOR


/**
 * WORTH_DIV2
 * └── 1...6: see enum SHAPE
 *     └── worth of each shape floor-divide by 2
 */
const short WORTH_DIV2[MAX_SHAPE] = { 0, 0, 1, 1, 2, 4, 0 };
short MAX_WORTH_DIV2 = 0; // sum of number of pieces of each shape except pawns * WORTH_DIV2 of each shape

/**
 * midgame/endgame_val (aka piece-square table)
 * └── 0,1: see enum PLAYER
 *     └── 1...6: see enum SHAPE
 *         └── 0...AREA-1: see main board
 *             └── worth of each shape based on midgame/endgame position with player at the bottom
 * 
 * values from Rofchade: http://www.talkchess.com/forum3/viewtopic.php?f=2&t=68311&start=19
 */
short MIDGAME_WORTH[MAX_PLAYER][MAX_SHAPE][AREA] = {
    {0}, // black initialize later
    {
        {0},
        {   // pawn
            82,  82,  82,  82,  82,  82,  82,  82, 0, 0, 
            180, 216, 143, 177, 150, 208, 116,  71, 0, 0, 
            76,  89, 108, 113, 147, 138, 107,  62, 0, 0, 
            68,  95,  88, 103, 105,  94,  99,  59, 0, 0, 
            55,  80,  77,  94,  99,  88,  92,  57, 0, 0, 
            56,  78,  78,  72,  85,  85, 115,  70, 0, 0, 
            47,  81,  62,  59,  67, 106, 120,  60, 0, 0, 
            82,  82,  82,  82,  82,  82,  82,  82, 0, 0, 
        },
        {   // knight
            170, 248, 303, 288, 398, 240, 322, 230, 0, 0, 
            264, 296, 409, 373, 360, 399, 344, 320, 0, 0, 
            290, 397, 374, 402, 421, 466, 410, 381, 0, 0, 
            328, 354, 356, 390, 374, 406, 355, 359, 0, 0, 
            324, 341, 353, 350, 365, 356, 358, 329, 0, 0, 
            314, 328, 349, 347, 356, 354, 362, 321, 0, 0, 
            308, 284, 325, 334, 336, 355, 323, 318, 0, 0, 
            232, 316, 279, 304, 320, 309, 318, 314, 0, 0,
        },
        {   // bishop
            336, 369, 283, 328, 340, 323, 372, 357, 0, 0,
            339, 381, 347, 352, 395, 424, 383, 318, 0, 0,
            349, 402, 408, 405, 400, 415, 402, 363, 0, 0,
            361, 370, 384, 415, 402, 402, 372, 363, 0, 0, 
            359, 378, 378, 391, 399, 377, 375, 369, 0, 0,
            365, 380, 380, 380, 379, 392, 383, 375, 0, 0,
            369, 380, 381, 365, 372, 386, 398, 366, 0, 0,
            332, 362, 351, 344, 352, 353, 326, 344, 0, 0,
        },
        {   // rook
            509, 519, 509, 528, 540, 486, 508, 520, 0, 0,
            504, 509, 535, 539, 557, 544, 503, 521, 0, 0,
            472, 496, 503, 513, 494, 522, 538, 493, 0, 0,
            453, 466, 484, 503, 501, 512, 469, 457, 0, 0, 
            441, 451, 465, 476, 486, 470, 483, 454, 0, 0,
            432, 452, 461, 460, 480, 477, 472, 444, 0, 0,
            433, 461, 457, 468, 476, 488, 471, 406, 0, 0,
            458, 464, 478, 494, 493, 484, 440, 451, 0, 0,
        },
        {   // queen
            997, 1025, 1054, 1037, 1084, 1069, 1068, 1070, 0, 0,
            1001,  986, 1020, 1026, 1009, 1082, 1053, 1079, 0, 0, 
            1012, 1008, 1032, 1033, 1054, 1081, 1072, 1082, 0, 0,
            998,  998, 1009, 1009, 1024, 1042, 1023, 1026, 0, 0,
            1016,  999, 1016, 1015, 1023, 1021, 1028, 1022, 0, 0,
            1011, 1027, 1014, 1023, 1020, 1027, 1039, 1030, 0, 0,
            990, 1017, 1036, 1027, 1033, 1040, 1022, 1026, 0, 0,
            1024, 1007, 1016, 1035, 1010, 1000,  994,  975, 0, 0, 
        },
        {   // king
            -65,  23,  16, -15, -56, -34,   2,  13, 0, 0,
            29,  -1, -20,  -7,  -8,  -4, -38, -29, 0, 0,
            -9,  24,   2, -16, -20,   6,  22, -22, 0, 0,
            -17, -20, -12, -27, -30, -25, -14, -36, 0, 0,
            -49,  -1, -27, -39, -46, -44, -33, -51, 0, 0,
            -14, -14, -22, -46, -44, -30, -15, -27, 0, 0,
            1,   7,  -8, -64, -43, -16,   9,   8, 0, 0,
            -15,  36,  12, -54,   8, -28,  24,  14, 0, 0, 
        }
    }
};
short ENDGAME_WORTH[MAX_PLAYER][MAX_SHAPE][AREA] = {
    {0}, // black initialize later
    {
        {0},
        {   // pawn
            94,  94,  94,  94,  94,  94,  94,  94, 0, 0, 
            272, 267, 252, 228, 241, 226, 259, 281, 0, 0, 
            188, 194, 179, 161, 150, 147, 176, 178, 0, 0, 
            126, 118, 107,  99,  92,  98, 111, 111, 0, 0, 
            107, 103,  91,  87,  87,  86,  97,  93, 0, 0, 
            98, 101,  88,  95,  94,  89,  93,  86, 0, 0, 
            107, 102, 102, 104, 107,  94,  96,  87, 0, 0, 
            94,  94,  94,  94,  94,  94,  94,  94, 0, 0, 
        },
        {   // knight
            223, 243, 268, 253, 250, 254, 218, 182, 0, 0, 
            256, 273, 256, 279, 272, 256, 257, 229, 0, 0, 
            257, 261, 291, 290, 280, 272, 262, 240, 0, 0, 
            264, 284, 303, 303, 303, 292, 289, 263, 0, 0, 
            263, 275, 297, 306, 297, 298, 285, 263, 0, 0, 
            258, 278, 280, 296, 291, 278, 261, 259, 0, 0, 
            239, 261, 271, 276, 279, 261, 258, 237, 0, 0, 
            252, 230, 258, 266, 259, 263, 231, 217, 0, 0, 
        },
        {   // bishop
            283, 276, 286, 289, 290, 288, 280, 273, 0, 0,
            289, 293, 304, 285, 294, 284, 293, 283, 0, 0,
            299, 289, 297, 296, 295, 303, 297, 301, 0, 0,
            294, 306, 309, 306, 311, 307, 300, 299, 0, 0,
            291, 300, 310, 316, 304, 307, 294, 288, 0, 0,
            285, 294, 305, 307, 310, 300, 290, 282, 0, 0,
            283, 279, 290, 296, 301, 288, 282, 270, 0, 0, 
            274, 288, 274, 292, 288, 281, 292, 280, 0, 0,
        },
        {   // rook
            525, 522, 530, 527, 524, 524, 520, 517, 0, 0,
            523, 525, 525, 523, 509, 515, 520, 515, 0, 0, 
            519, 519, 519, 517, 516, 509, 507, 509, 0, 0,
            516, 515, 525, 513, 514, 513, 511, 514, 0, 0,
            515, 517, 520, 516, 507, 506, 504, 501, 0, 0,
            508, 512, 507, 511, 505, 500, 504, 496, 0, 0,
            506, 506, 512, 514, 503, 503, 501, 509, 0, 0,
            503, 514, 515, 511, 507, 499, 516, 492, 0, 0, 
        },
        {   // queen
            927,  958,  958,  963,  963,  955,  946,  956, 0, 0,
            919,  956,  968,  977,  994,  961,  966,  936, 0, 0,
            916,  942,  945,  985,  983,  971,  955,  945, 0, 0, 
            939,  958,  960,  981,  993,  976,  993,  972, 0, 0,
            918,  964,  955,  983,  967,  970,  975,  959, 0, 0,
            920,  909,  951,  942,  945,  953,  946,  941, 0, 0,
            914,  913,  906,  920,  920,  913,  900,  904, 0, 0,
            903,  908,  914,  893,  931,  904,  916,  895, 0, 0, 
        },
        {   // king
            -74, -35, -18, -18, -11,  15,   4, -17, 0, 0,
            -12,  17,  14,  17,  17,  38,  23,  11, 0, 0,
            10,  17,  23,  15,  20,  45,  44,  13, 0, 0,
            -8,  22,  24,  27,  26,  33,  26,   3, 0, 0,
            -18,  -4,  21,  24,  27,  23,   9, -11, 0, 0,
            -19,  -3,  11,  21,  23,  16,   7,  -9, 0, 0,
            -27, -11,   4,  13,  14,   4,  -5, -17, 0, 0,
            -53, -34, -21, -11, -28, -14, -24, -43, 0, 0, 
        }
    }
};

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
    { '_', 'p', 'n', 'b', 'r', 'q', 'k' },
    { '_', 'P', 'N', 'B', 'R', 'Q', 'K' },
};

/**
 * IMPORTANT: Before calling, use board[ind] to check tile is NOT empty since return 0 can = black or empty!
 * @return the player occupying the given tile (see enum PLAYER).
 */
inline bool player_of(short tile)
{
    return tile / 100; // extract digit1
}

inline short shape_of(short tile)
{
    return (tile / 10) % 10; // extract digit2
}

inline short indB2_of(short tile)
{
    return tile % 10; // extract digit3
}

inline short x_of(short ind)
{
    return ind % WIDTH;
}

inline short y_of(short ind)
{
    return ind / WIDTH;
}

/**
 * @return the forward direction relative to the given player.
 * With WHITE at the bottom and black at the top, white's foward = -WIDTH, black's foward = +WIDTH.
 */
inline short rel_foward(bool player)
{
    return (player ? -1 : 1)*WIDTH;
}

inline bool is_play_area(short ind)
{
    return x_of(ind) < 8 && unsigned(ind) < AREA; // if x < 0, it becomes a huge unsigned number > AREA.
}

/**
 * Initialize NUM, MAX_WORTH_DIV2, board2, Zplayer, Ztable, hash, black in MIDGAME/ENDGAME_WORTH.
 */
void init_all()
{
    // fill board2 with -1
    for (short player = BLACK; player <= WHITE; player++)
    {
        for (short shape = PAWN; shape < MAX_SHAPE; shape++)
        {
            for (short &ind : board2[player][shape])
                ind = -1;
        }
    }

    // board2, NUM
    short num_temp[MAX_PLAYER][MAX_SHAPE] = {{0}};
    for (short ind = 0; ind < AREA; ind++)
    {
        short tile = board[ind];
        if (tile)
        {
            bool player = player_of(tile);
            short shape = shape_of(tile);
            board2[player][shape][indB2_of(tile)] = ind;
            num_temp[player][shape] ++;
        }
    }
    for (int shape = PAWN; shape < MAX_SHAPE; shape++)
        NUM[shape] = std::max(num_temp[BLACK][shape], num_temp[WHITE][shape]);

    // MAX_WORTH_DIV2
    for (short player = BLACK; player <= WHITE; player++)
    {
        for (short shape = KNIGHT; shape < KING; shape++)
            MAX_WORTH_DIV2 += WORTH_DIV2[shape]*NUM[shape];
    }

    // Zplayer, Ztable
    Zplayer = rng();
    for (short ind = 0; ind < AREA; ind++)
    {
        for (short player = BLACK; player <= WHITE; player++)
        {
            for (short shape = PAWN; shape < MAX_SHAPE; shape++)
                Ztable[ind][player][shape] = rng();
        }
    }

    // hash
    for (short player = BLACK; player <= WHITE; player++)
    {
        for (short shape = PAWN; shape < MAX_SHAPE; shape++)
        {
            for (short indB2 = 0; indB2 < NUM[shape]; indB2++)
            {
                short ind = board2[player][shape][indB2];
                hash ^= Ztable[ind][player][shape];
            }
        }
    }

    // black in MIDGAME/ENDGAME_WORTH = white mirrored along x-axis
    for (short shape = PAWN; shape < MAX_SHAPE; shape++)
    {
        for (short ind = 0; ind < AREA; ind++)
        {
            short mirror = (HEIGHT-1 - y_of(ind))*WIDTH + x_of(ind);
            MIDGAME_WORTH[BLACK][shape][ind] = MIDGAME_WORTH[WHITE][shape][mirror];
            ENDGAME_WORTH[BLACK][shape][ind] = ENDGAME_WORTH[WHITE][shape][mirror];

        }
    }
}

/**
 * @return captured tile.
 * When undoing moves,
 * 1. returned value is directly fed into parameter: restore.
 * 2. ind_i and ind_f switch places.
 */
inline short move(bool player, short shape, short indB2, short ind_i, short ind_f, short restore = 0)
{
    short capture = board[ind_f];
    if (capture)
    {
        short shape_capture = shape_of(capture);
        board2[!player][shape_capture][indB2_of(capture)] -= AREA; // move ind_f outside board
        hash ^= Ztable[ind_f][!player][shape_capture];
    }

    else if (restore) // if tile to restore exists
    {
        short shape_restore = shape_of(restore);
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
inline bool can_capture(bool player, short tile)
{
    return player_of(tile) != player && shape_of(tile) != KING;
}

std::vector<short> gen_moves(bool player, short shape, short ind_i)
{
    std::vector<short> moves;
    moves.reserve(27);
    short ind = 0, tile = 0;
    
    if (shape == KING)
    {
        for (short v: K_VECTOR)
        {
            ind = ind_i + v;
            tile = board[ind];
            if (is_play_area(ind) && (!tile || can_capture(player, tile)))
                moves.emplace_back(ind);
        }
    }
    if (shape == KNIGHT)
    {
        for (short v: N_VECTOR)
        {
            ind = ind_i + v;
            tile = board[ind];
            if (is_play_area(ind) && (!tile || can_capture(player, tile)))
                moves.emplace_back(ind);
        }
    }
    if (shape == ROOK || shape == QUEEN)
    {
        for (short v: R_VECTOR)
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
        for (short v: B_VECTOR)
        {
            for (ind = ind_i + v; is_play_area(ind) && !board[ind]; ind += v)
                moves.emplace_back(ind);
            
            if (is_play_area(ind) && can_capture(player, board[ind]))
                moves.emplace_back(ind);
        }
    }
    if (shape == PAWN)
    {
        short y_i = y_of(ind_i), dy = rel_foward(player);
        if (1 <= y_i && y_i <= 6)
        {
            ind = ind_i + dy;

            // capture moves
            for (short dx = -1; dx <= 1; dx += 2)
            {
                short ind_f = ind + dx;
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
inline bool is_path_clear(short ind_i, short ind_f, short dx, short dy)
{
    short v = (ind_f - ind_i) / std::max(dx, dy); 
    for (short ind = ind_i + v; ind != ind_f; ind += v)
    {
        if (board[ind])
            return false;
    }
    return true;
}

/**
 * @return whether the given tile is currently attacked by the enemy of the given player.
 */
bool is_attacked(bool player, short ind)
{
    short ind_e = 0, // enemy index on main board
        dx = 0, dy = 0;

    // Start with pieces more likely to be attacked by (pawns -> knights -> bishops -> rooks -> queens -> king).

    // check for enemy pawns
    // Pretend I'm a pawn and check where I can capture.
    ind_e = ind + rel_foward(player);
    for (short dx = -1; dx <= 1; dx += 2)
    { 
        short tile = board[ind_e + dx];
        if (player_of(tile) == !player && shape_of(tile) == PAWN)
            return 1;
    }

    // check for enemy knights
    // Iterate over all enemy knights.
    for (short indB2_e = 0; indB2_e < NUM[KNIGHT]; indB2_e++)
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
    for (short indB2_e = 0; indB2_e < NUM[BISHOP]; indB2_e++)
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
    for (short indB2_e = 0; indB2_e < NUM[ROOK]; indB2_e++)
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
        short i = 0;
        for (Ttable_entry entry : Ttable)
        {
            if (i < 10)
            {
                if (entry.hash)
                {
                    std::cout << "  " << entry.hash << ", " << entry.score << ", " << entry.depth << '\n';
                    i++;
                }
            }
            else
            {
                std::cout << "  ... (" << TTABLE_SZ - 10 << " more)" << std::endl;
                break;
            }
        }
    }

    if (has_hash)
        std::cout << "Hash:" << std::endl << "  " << hash << std::endl;

    if (has_board2)
    {
        std::cout << "Board2:" << std::endl;
        for (short player = BLACK; player <= WHITE; player++)
        {
            for (short shape = PAWN; shape < MAX_SHAPE; shape++)
            {
                std::cout << std::setw(3) << '[' << char_of[player][shape] << "]=";
                for (short indB2 = 0; indB2 < NUM[shape]; indB2++)
                {
                    short ind = board2[player][shape][indB2];
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
            for (short ind = 0; ind < AREA; ind++)
            {
                short tile = board[ind];
                if (tile && player_of(tile) != player && is_attacked(!player, ind))
                    std::cout << ind << ", ";
            }
            std::cout << std::endl;
        }
    }

    std::cout << "   +-------------BOT-------------+" << std::endl;
    for (short ind = 0; ind < AREA; ind++)
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

            short tile = board[ind];
            std::cout << std::setw(3) << char_of[player_of(tile)][shape_of(tile)];
        }
    }
    std::cout << "   +-------------YOU-------------+" << std::endl;
}

short approx(bool player)
{
    short midgame_worths[MAX_PLAYER] = {0, 0}, endgame_worths[MAX_PLAYER] = {0, 0};
    float worths = 0; // sum of worth of remaining pieces

    for (short player = BLACK; player <= WHITE; player++)
    {
        for (short shape = KNIGHT; shape < KING; shape++) // start from knight since WORTH_DIV2[PAWN] = 0
        {
            for (short indB2 = 0; indB2 < NUM[shape]; indB2++)
            {
                short ind = board2[player][shape][indB2];
                if (ind >= 0) // if not captured
                {
                    midgame_worths[player] += MIDGAME_WORTH[player][shape][ind];
                    endgame_worths[player] += ENDGAME_WORTH[player][shape][ind];
                    worths += WORTH_DIV2[shape];
                }
            }
        }
    }

    short midgame_score = midgame_worths[player] - midgame_worths[!player],
        endgame_score = endgame_worths[player] - endgame_worths[!player];

    return endgame_score - (worths/MAX_WORTH_DIV2)*(endgame_score - midgame_score); // linear interpolation
}

/**
 * Minimax + Tapered Piece-Square Table Evaluation + AlphaBeta Prunning + Zobrist Hashed Transposition Table
 * In first call, use depth = 1, alpha = SHRT_MIN, beta = SHRT_MAX.
 * @return
 *      n = 0 if draw.
 *      n ∈ [-MAX_WORTH_DIV2, 0) U (0, MAX_WORTH_DIV2] if over MAX_DEPTH.
 *      n = SHRT_MAX if checked by MAXER.
 *      n = SHRT_MIN if checked by MINER.
 */
short minimax(bool player, short depth, short alpha, short beta)
{
    Ttable_entry &entry = Ttable[hash % TTABLE_SZ];
    if (entry.hash == hash)
        return entry.score;

    short capture = 0, score = 0, child_score = 0;
    bool has_move = false, is_maxer = player;

    if (depth > MAX_DEPTH)
        return approx(player) * (is_maxer ? 1 : -1);

    for (short shape = PAWN; shape < MAX_SHAPE; shape++)
    {
        for (short indB2 = 0; indB2 < NUM[shape]; indB2++)
        {
            short ind_i = board2[player][shape][indB2];
            if (ind_i >= 0) // if not captured
            {
                for (short ind_f : gen_moves(player, shape, ind_i))
                {
                    capture = move(player, shape, indB2, ind_i, ind_f);

                    if (!is_attacked(player, board2[player][KING][0]))
                    {
                        has_move = true;
                        child_score = minimax(!player, depth+1, alpha, beta);

                        if (is_maxer)
                            alpha = std::max(alpha, child_score);
                        else
                            beta = std::min(beta, child_score);

                        if (beta <= alpha)
                        {
                            move(player, shape, indB2, ind_f, ind_i, capture); // undo move
                            score = (is_maxer) ? alpha : beta;
                            if (depth > entry.depth)
                                entry = {hash, score, depth};
                            return score;
                        }
                    }
                    move(player, shape, indB2, ind_f, ind_i, capture); // undo move
                }
            }
        }
    }

    if (has_move)
        score = (is_maxer) ? alpha : beta;

    else if (is_attacked(player, board2[player][KING][0])) // checkmated
        score = (is_maxer) ? SHRT_MIN : SHRT_MAX;
    
    if (depth > entry.depth)
        entry = {hash, score, depth};
    return score;
}

/**
 * Behave the same as minimax(), except with recording the best move and without recording to Ttable and AlphaBeta Prunning.
 * @return 0: bot has move(s). 1: bot is checkmated. 2: bot is stalemated.
 */
short bot_move(bool player)
{
    short capture = 0, child_score = 0, best_indB2 = 0, best_shape = 0, best_ind_i = 0, best_ind_f = 0, best_score = (player) ? SHRT_MIN : SHRT_MAX;
    bool has_move = false, is_maxer = player;

    for (short shape = PAWN; shape < MAX_SHAPE; shape++)
    {
        for (short indB2 = 0; indB2 < NUM[shape]; indB2++)
        {
            short ind_i = board2[player][shape][indB2];
            if (ind_i >= 0) // if not captured
            {
                std::cout << "Possible {move, score} from " << ind_i << ": ";
                for (short ind_f : gen_moves(player, shape, ind_i))
                {
                    capture = move(player, shape, indB2, ind_i, ind_f);
                    
                    if (!is_attacked(player, board2[player][KING][0]))
                    {
                        has_move = true;
                        child_score = minimax(!player, 1, SHRT_MIN, SHRT_MAX);
                    
                        std::cout << "{" << ind_f << ", " << child_score << "}, ";
                        if (is_maxer)
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
                    }
                    move(player, shape, indB2, ind_f, ind_i, capture); // undo move
                }
                std::cout << std::endl;
            }
        }
    }
    if (has_move)
    {
        std::cout << "Chosen move: " << best_ind_i << " to " << best_ind_f << std::endl;
        move(player, best_shape, best_indB2, best_ind_i, best_ind_f);
        return 0;
    }
    else if (!has_move && is_attacked(player, board2[player][KING][0])) // checkmated
        return 1;
    
    else
        return 2;
}

/**
 * @return whether move is valid.
 * 0: valid, 1: broken chess rule, 2: empty initial tile, 3: outside play area, 4: friendly fire, 5: blocked path
 * TODO: 5: impersonating bot, 6: checkmate, 7: draw
 */
short validate(short ind_i, short ind_f)
{
    bool player = player_of(board[ind_i]);
    short shape = shape_of(board[ind_i]),
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
    short ind_i = 0, ind_f = 0, invalid = 0;
    std::vector<short> inds_f;

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

        short outcome = bot_move(BLACK);
        if (outcome == 1)
            std::cout << "You won!\nPC: NOOO MY DIGNITY!" << std::endl;
        else if (outcome == 2)
            std::cout << "Ended in draw.\nPC: You\'ll never win ... not satisfied? Replay!" << std::endl;
        if (outcome)
            break;
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
