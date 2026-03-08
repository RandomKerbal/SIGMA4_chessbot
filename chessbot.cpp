#include <vector>
#include <climits>
#include <random>

// remove in VEXIQ
#include <iostream>
#include <iomanip>

std::mt19937_64 rng(0);

const short MAX_DEPTH = 6;

enum PLAYER: short {
    BLACK = 0, WHITE = 1,
    MAX_PLAYER = 2,
    BOT = BLACK, HUMAN = WHITE,
};
enum SHAPE: short {
    PAWN = 0, KNIGHT = 1, BISHOP = 2, ROOK = 3, QUEEN = 4, KING = 5,
    MAX_SHAPE = 6
};
/**
 * SHAPE_PHASE
 * └── 0...5: see enum SHAPE
 *     └── how much each shape contributes to the game's phase
 * 
 * values from Rofchade: http://www.talkchess.com/forum3/viewtopic.php?f=2&t=68311&start=19
 */
const short SHAPE_PHASE[MAX_SHAPE] = { 0, 1, 1, 2, 4, 0 };

const short HEIGHT = 8, WIDTH = HEIGHT + 2, AREA = HEIGHT*WIDTH;

struct BoardEntry {
    PLAYER player = BLACK;
    SHAPE shape = PAWN;
    short sq = -1;
};
/**
 * board
 * └── 0,1: see enum PLAYER
 *     └── 0...32 (arbitrary): max total number of pieces of each player
 *         └── entry: { player, shape, sq }
 * 
 * IMPORTANT: Shape order of entries must match enum SHAPE! 
 * If an entry is captured, its sq = sq - AREA. If an entry is empty, its sq = -1.
 */
BoardEntry board[MAX_PLAYER][32] = {
    {   // black
        { BLACK, PAWN,  10 }, { BLACK, PAWN,  11 }, { BLACK, PAWN, 12 }, { BLACK, PAWN, 13 }, { BLACK, PAWN, 14 }, { BLACK, PAWN, 15 }, { BLACK, PAWN, 16 }, { BLACK, PAWN, 17 }, // pawn
        { BLACK, KNIGHT, 1 }, { BLACK, KNIGHT, 6 }, // knight
        { BLACK, BISHOP, 2 }, { BLACK, BISHOP, 5 }, // bishop
        { BLACK, ROOK,   0 }, { BLACK, ROOK,   7 }, // rook
        { BLACK, QUEEN,  3 }, // queen
        { BLACK, KING,   4 }  // king
    },
    {   // white
        { WHITE, PAWN,   60 }, { WHITE, PAWN,   61 }, { WHITE, PAWN, 62 }, { WHITE, PAWN, 63 }, { WHITE, PAWN, 64 }, { WHITE, PAWN, 65 }, { WHITE, PAWN, 66 }, { WHITE, PAWN, 67 },
        { WHITE, KNIGHT, 71 }, { WHITE, KNIGHT, 76 },
        { WHITE, BISHOP, 72 }, { WHITE, BISHOP, 75 },
        { WHITE, ROOK,   70 }, { WHITE, ROOK,   77 },
        { WHITE, QUEEN,  73 },
        { WHITE, KING,   74 }
    }
};

/**
 * BEGIN
 * └── 0,1: see enum PLAYER
 *     └── 0...5: see enum SHAPE
 *         └── beginning index on the board for each shape of each player.
 * 
 * Since KING is the last shape, BEGIN[player][KING] = used size of board.
 */
short BEGIN[MAX_PLAYER][MAX_SHAPE] = {{0}};

/** 
 * squares
 * ├── columns 8,9 element
 * │   └── nullptr as sentinel
 * │
 * └── columns 0...8 element
 *     └── pointer to an entry in the main board, else nullptr as empty square.
 * 
 * The board is 10x8 since the two rightmost columns are sentinels that prevent
 * pointers from "wrapping" onto the previous/next row. The real playable area
 * is the leftmost 8x8. Sentinels/empties are nullptr to identify easily using !squares[sq].
 */
BoardEntry* squares[AREA] = {nullptr};

/**
 * Ztable (Zobrist Table)
 * └── 0,1: see enum PLAYER
 *     └── 0...5: see enum SHAPE
 *         └── 0...80: index on main board
 *             └── random 64-bit integer
 */
unsigned long long Ztable[MAX_PLAYER][MAX_SHAPE][AREA] = {{{0}}};
unsigned long long ZBLACK = 0;
unsigned long long hash = 0;

const unsigned int TABLE_SZ = 4194304; // 2^22
struct TtableEntry {
    unsigned long long hash = 0;
    short score = 0;
    short phase = SHRT_MAX;
};
TtableEntry t_table[TABLE_SZ]; // Transposition Table
unsigned long long m_table[MAX_DEPTH]; // Move History Table

float PHASE_OPENING = 0; // sum of SHAPE_PHASE of all initial pieces
short phase = 0; // sum of SHAPE_PHASE of all current pieces
short worth_opening[MAX_PLAYER] = {0, 0};
short worth_endgame[MAX_PLAYER] = {0, 0};

/**
 * WORTH_OPENING/ENDGAME (aka piece-square table)
 * └── 0,1: see enum PLAYER
 *     └── 0...5: see enum SHAPE
 *         └── 0...AREA-1: see main board
 *             └── worth of each shape based on opening/endgame position with player at the bottom
 * 
 * values from Rofchade: http://www.talkchess.com/forum3/viewtopic.php?f=2&t=68311&start=19
 */
short WORTH_OPENING[MAX_PLAYER][MAX_SHAPE][AREA] = {
    {}, // black initialize later
    {
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
short WORTH_ENDGAME[MAX_PLAYER][MAX_SHAPE][AREA] = {
    {}, // black initialize later
    {
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
 * char_of
 * └── 0,1: see enum PLAYER
 *     └── 0...5: see enum SHAPE
 * 
 * Converts entry from integer to character.
 * lowercase: black; uppercase: white 
 */
const char char_of[MAX_PLAYER][MAX_SHAPE] = {
    { 'p', 'n', 'b', 'r', 'q', 'k' },
    { 'P', 'N', 'B', 'R', 'Q', 'K' },
};

inline short x_of(short sq)
{
    return sq % WIDTH;
}

inline short y_of(short sq)
{
    return sq / WIDTH;
}

/**
 * @return the forward direction relative to the given player.
 * With WHITE at the bottom and black at the top, white's foward = -WIDTH, black's foward = +WIDTH.
 */
inline short rel_foward(PLAYER player)
{
    return (player ? -1 : 1)*WIDTH;
}

inline bool is_play_area(short sq)
{
    return x_of(sq) < 8 && unsigned(sq) < AREA; // if x < 0, it becomes a huge unsigned number > AREA.
}

void init_all()
{
    // black's WORTH_OPENING/ENDGAME = white's mirrored along x-axis
    for (short shape = PAWN; shape < MAX_SHAPE; shape++)
    {
        for (short sq = 0; sq < AREA; sq++)
        {
            short sq_mirror = (HEIGHT-1 - y_of(sq))*WIDTH + x_of(sq);
            WORTH_OPENING[BLACK][shape][sq] = WORTH_OPENING[WHITE][shape][sq_mirror];
            WORTH_ENDGAME[BLACK][shape][sq] = WORTH_ENDGAME[WHITE][shape][sq_mirror];

        }
    }

    for (short player = BLACK; player <= WHITE; player++)
    {
        // squares, BEGIN, PHASE_OPENING, worth, worth_opening, worth_endgame
        SHAPE prev_shape = PAWN, shape = PAWN;
        for (short ind = 0; ind < 32; ind++)
        {
            BoardEntry &entry = board[player][ind];
            short sq = entry.sq;
            if (sq >= 0)
            {
                shape = entry.shape;
                if (shape > prev_shape)
                    BEGIN[player][shape] = ind;

                squares[sq] = &entry;
                phase += SHAPE_PHASE[shape];
                worth_opening[player] += WORTH_OPENING[player][shape][sq];
                worth_endgame[player] += WORTH_ENDGAME[player][shape][sq];
                prev_shape = shape;
            }
        }
        PHASE_OPENING = phase;

        // Ztable
        for (short shape = PAWN; shape < MAX_SHAPE; shape++)
        {
            for (short sq = 0; sq < AREA; sq++)
            {
                if (is_play_area(sq))
                    Ztable[player][shape][sq] = rng();
            }
        }
    }
    ZBLACK = rng();

    // hash
    for (short sq = 0; sq < AREA; sq++)
    {
        if (squares[sq])
        {
            BoardEntry &entry = *squares[sq];
            hash ^= Ztable[entry.player][entry.shape][sq];
        }
    }
}

/**
 * @return pointer to captured entry.
 * When undoing moves,
 * 1. returned value is directly fed into parameter: add.
 * 2. sq_i and sq_f switch places.
 */
inline BoardEntry *move(PLAYER player, SHAPE shape, short sq_i, short sq_f, BoardEntry *add = nullptr)
{
    BoardEntry *del = squares[sq_f];
    if (del)
    {
        BoardEntry &del_entry = *del;
        del_entry.sq -= AREA; // move sq outside board

        PLAYER del_player = del_entry.player;
        SHAPE del_shape = del_entry.shape;
        hash ^= Ztable[del_player][del_shape][sq_f];
        phase -= SHAPE_PHASE[del_shape];
        worth_opening[del_player] -= WORTH_OPENING[del_player][del_shape][sq_f];
        worth_endgame[del_player] -= WORTH_ENDGAME[del_player][del_shape][sq_f];
    }

    else if (add)
    {
        BoardEntry &add_entry = *add;
        add_entry.sq += AREA; // move sq back inside

        PLAYER add_player = add_entry.player;
        SHAPE add_shape = add_entry.shape;
        hash ^= Ztable[add_player][add_shape][sq_i];
        phase += SHAPE_PHASE[add_shape];
        worth_opening[add_player] += WORTH_OPENING[add_player][add_shape][sq_i];
        worth_endgame[add_player] += WORTH_ENDGAME[add_player][add_shape][sq_i];
    }

    (*squares[sq_i]).sq = sq_f;

    // add to final square
    squares[sq_f] = squares[sq_i];
    hash ^= Ztable[player][shape][sq_f];
    worth_opening[player] += WORTH_OPENING[player][shape][sq_f];
    worth_endgame[player] += WORTH_ENDGAME[player][shape][sq_f];
    
    // remove from initial square
    squares[sq_i] = add;
    hash ^= Ztable[player][shape][sq_i];
    worth_opening[player] -= WORTH_OPENING[player][shape][sq_i];
    worth_endgame[player] -= WORTH_ENDGAME[player][shape][sq_i];

    // switch player of hash
    hash ^= ZBLACK;
    return del;
}

/**
 * @return whether the given entry has the foe of the given player && not king.
 */
inline bool can_capture(PLAYER player, PLAYER capture_player, SHAPE capture_shape)
{
    return capture_player != player && capture_shape != KING;
}

inline void MVV_sort_push(std::vector<short> &moves, short &quiet_begin, SHAPE &max_capture_shape, SHAPE capture_shape, short sq)
{
    moves.emplace_back(sq);
    if (capture_shape > max_capture_shape)
    {
        max_capture_shape = capture_shape;
        std::swap(moves.front(), moves.back());
    }

    std::swap(moves[quiet_begin], moves.back());
    quiet_begin++;
}

std::vector<short> gen_moves(PLAYER player, SHAPE shape, short sq_i)
{
    std::vector<short> moves;
    moves.reserve(27);
    BoardEntry capture;
    SHAPE capture_shape = PAWN, max_capture_shape = PAWN;
    short sq = 0, quiet_begin = 0;
    
    // MVV: start with capturing most worthy shape, then quiet moves.
    if (shape == PAWN)
    {
        short y_i = y_of(sq_i);
        if (1 <= y_i && y_i <= 6)
        {
            short dy = rel_foward(player);
            sq = sq_i + dy;

            // capture moves
            for (short dx = -1; dx <= 1; dx += 2)
            {
                short sq_capture = sq + dx;
                if (squares[sq_capture])
                {
                    capture = *squares[sq_capture];
                    capture_shape = capture.shape;
                    if (can_capture(player, capture.player, capture_shape))
                        MVV_sort_push(moves, quiet_begin, max_capture_shape, capture_shape, sq_capture);
                }
            }

            // y-moves
            if (!squares[sq])
            {
                moves.emplace_back(sq);
                if ((y_i == 1 && dy > 0) || (y_i == 6 && dy < 0)) // if pawn can step 2
                {
                    sq += dy;
                    if (!squares[sq])
                        moves.emplace_back(sq);
                }
            }
        }
    }
    else if (shape == KNIGHT)
    {
        for (short v: N_VECTOR)
        {
            sq = sq_i + v;
            if (is_play_area(sq))
            {
                if (!squares[sq])
                    moves.emplace_back(sq);
                else
                {
                    capture = *squares[sq];
                    capture_shape = capture.shape;
                    if (can_capture(player, capture.player, capture_shape))
                        MVV_sort_push(moves, quiet_begin, max_capture_shape, capture_shape, sq);
                }
            }
        }
    }
    else if (shape == KING)
    {
        for (short v: K_VECTOR)
        {
            sq = sq_i + v;
            if (is_play_area(sq))
            {
                if (!squares[sq])
                    moves.emplace_back(sq);
                else
                {
                    capture = *squares[sq];
                    capture_shape = capture.shape;
                    if (can_capture(player, capture.player, capture_shape))
                        MVV_sort_push(moves, quiet_begin, max_capture_shape, capture_shape, sq);
                }
            }
        }
    }
    if (shape == BISHOP || shape == QUEEN)
    {
        for (short v: B_VECTOR)
        {
            for (sq = sq_i + v; is_play_area(sq) && !squares[sq]; sq += v)
                moves.emplace_back(sq);
            
            if (is_play_area(sq))
            {
                capture = *squares[sq];
                capture_shape = capture.shape;
                if (can_capture(player, capture.player, capture_shape))
                    MVV_sort_push(moves, quiet_begin, max_capture_shape, capture_shape, sq);
            }
        }
    }
    if (shape == ROOK || shape == QUEEN)
    {
        for (short v: R_VECTOR)
        {
            // travel until on top of a shape
            for (sq = sq_i + v; is_play_area(sq) && !squares[sq]; sq += v)
                moves.emplace_back(sq);

            if (is_play_area(sq))
            {
                capture = *squares[sq];
                capture_shape = capture.shape;
                if (can_capture(player, capture.player, capture_shape))
                    MVV_sort_push(moves, quiet_begin, max_capture_shape, capture_shape, sq);
            }
        }
    }

    return moves;
}

/**
 * @return whether all entries on the path are empty.
 * IMPORTANT: dx, dy are absolute.
 */
inline bool is_path_clear(short sq_i, short sq_f, short dx, short dy)
{
    short v = (sq_f - sq_i) / std::max(dx, dy);
    for (short sq = sq_i + v; sq != sq_f; sq += v)
    {
        if (squares[sq])
            return false;
    }
    return true;
}

/**
 * @return whether the given square is currently attacked by the foe of the given player.
 */
bool is_attacked(PLAYER player, short sq)
{
    short i = 0, ind = 0, dx = 0, dy = 0, y = 0, sq_foe = 0;

    // check for foe pawns by being a pawn and check where I can capture
    y = y_of(sq);
    if (1 <= y && y <= 6)
    {
        sq_foe = sq + rel_foward(player);
        for (dx = -1; dx <= 1; dx += 2)
        {
            short sq_capture = sq_foe + dx;
            if (squares[sq_capture])
            {
                BoardEntry capture = *squares[sq_capture];
                if (capture.player != player && capture.shape == PAWN)
                    return 1;
            }
        }
    }

    // check for foe knights by iterating over all foe knights
    for (ind = BEGIN[!player][KNIGHT]; ind < BEGIN[!player][BISHOP]; ind++)
    {
        sq_foe = board[!player][ind].sq;
        if (sq_foe >= 0) // if not captured
        {
            dx = abs(x_of(sq_foe) - x_of(sq));
            dy = abs(y_of(sq_foe) - y_of(sq));
            if ((dx == 1 && dy == 2) || (dx == 2 && dy == 1))
                return 1;
        }
    }

    // check for foe bishops
    for (ind = BEGIN[!player][BISHOP]; ind < BEGIN[!player][ROOK]; ind++)
    {
        sq_foe = board[!player][ind].sq;
        if (sq_foe >= 0)
        {
            dx = abs(x_of(sq_foe) - x_of(sq));
            dy = abs(y_of(sq_foe) - y_of(sq));
            if (dx == dy && is_path_clear(sq, sq_foe, dx, dy))
                return 1;
        }
    }

    // check for foe rooks
    for (ind = BEGIN[!player][ROOK]; ind < BEGIN[!player][QUEEN]; ind++)
    {
        sq_foe = board[!player][ind].sq;
        if (sq_foe >= 0)
        {
            dx = abs(x_of(sq_foe) - x_of(sq));
            dy = abs(y_of(sq_foe) - y_of(sq));
            if ((dx == 0 || dy == 0) && is_path_clear(sq, sq_foe, dx, dy))
                return 1;
        }
    }

    // check for foe queen
    sq_foe = board[!player][BEGIN[!player][QUEEN]].sq;
    if (sq_foe >= 0)
    {
        dx = abs(x_of(sq_foe) - x_of(sq));
        dy = abs(y_of(sq_foe) - y_of(sq));
        if ((dx == dy || dx == 0 || dy == 0) && is_path_clear(sq, sq_foe, dx, dy))
            return 1;
    }

    // check for foe king
    sq_foe = board[!player][BEGIN[!player][KING]].sq;
    dx = abs(x_of(sq_foe) - x_of(sq));
    dy = abs(y_of(sq_foe) - y_of(sq));
    if (std::max(dx, dy) == 1)
        return 1;

    return 0;
}

void out_board(bool has_t_table = false, bool has_hash = false, bool has_index = false, bool has_phase = false, bool has_worth = false, bool has_attack = false)
{
    const char TAB[4] = "   ";
    if (has_t_table)
    {
        std::cout << "Transposition Table:" << std::endl;
        short i = 0;
        for (TtableEntry t_entry : t_table)
        {
            if (i < 10)
            {
                if (t_entry.hash)
                {
                    std::cout << TAB << t_entry.hash << ", " << t_entry.score << ", " << t_entry.phase << std::endl;
                    i++;
                }
            }
            else
                break;
        }
        std::cout << TAB << "... (" << TABLE_SZ - 10 << " more)" << std::endl;
    }

    if (has_hash)
        std::cout << "hash:" << std::endl << TAB << hash << std::endl;

    if (has_index)
    {
        std::cout << "Indexes: " << std::endl << TAB;
        for (short player = BLACK; player <= WHITE; player++)
        {
            for (short ind = 0; ind <= BEGIN[player][KING]; ind++)
            {
                BoardEntry &board_entry = board[player][ind];
                std::cout << '[' << char_of[player][board_entry.shape] << "]=";
                std::cout << board_entry.sq << ", ";
            }
        }
        std::cout << std::endl;
    }

    if (has_phase)
        std::cout << "Distance from endgame:" << std::endl << TAB << phase << '/' << PHASE_OPENING << std::endl;

    if (has_worth)
    {
        std::cout << "Worth as opening:" << std::endl;
        for (short player = BLACK; player <= WHITE; player++)
        {
            std::cout << TAB << '[' << (player ? "WHITE" : "BLACK") << "]=" << worth_opening[player] << std::endl;
        }
        std::cout << "Worth as endgame:" << std::endl;
        for (short player = BLACK; player <= WHITE; player++)
        {
            std::cout << TAB << '[' << (player ? "WHITE" : "BLACK") << "]=" << worth_endgame[player] << std::endl;
        }
    }


    if (has_attack)
    {
        std::cout << "Tile(s) attacked by:" << std::endl;
        for (short player = BLACK; player <= WHITE; player++)
        {
            std::cout << TAB << '[' << (player ? "WHITE" : "BLACK") << "]=";
            for (short sq = 0; sq < AREA; sq++)
            {
                BoardEntry *ptr = squares[sq];
                if (ptr && (*ptr).player != player && is_attacked(PLAYER(!player), (*ptr).sq))
                    std::cout << sq << ", ";
            }
            std::cout << std::endl;
        }
    }

    std::cout << TAB << "+-------------BOT-------------+" << std::endl;
    for (short sq = 0; sq < AREA; sq++)
    {
        if (x_of(sq) > 7) // sentinels
        {
            std::cout << " . . | " << sq - 1 << std::endl;
            sq++;
        }
        else
        {
            if (x_of(sq) == 0)
                std::cout << std::setw(2) << sq << " |";

            BoardEntry *ptr = squares[sq];
            if (ptr)
                std::cout << std::setw(3) << char_of[(*ptr).player][(*ptr).shape];
            else
                std::cout << std::setw(3) << '_';
        }
    }
    std::cout << TAB << "+-------------YOU-------------+" << std::endl;
}

inline short approx(PLAYER player)
{
    short score_opening = worth_opening[player] - worth_opening[!player],
          score_endgame = worth_endgame[player] - worth_endgame[!player];

    return score_endgame - (phase/PHASE_OPENING)*(score_endgame - score_opening); // linear interpolation
}

inline bool is_repeat3(short depth)
{
    // no need to check [depth-1], [depth-2], [depth-3]
    if (depth >= 4 && hash == m_table[depth-4])
        // (depth >= 6 && hash == m_table[depth-6])
        return true;
    else
        return false;
}

/**
 * Minimax + Tapered Piece-Square Table Evaluation + AlphaBeta Prunning + Zobrist Hashing Transposition Table + MVV LVA + Threefold Repetition Check
 * In first call, use depth = 0, alpha = SHRT_MIN, beta = SHRT_MAX.
 * @return
 *      n = 0 if draw.
 *      n ∈ [-PHASE_OPENING, 0) U (0, PHASE_OPENING] if over MAX_DEPTH.
 *      n = SHRT_MAX if checked by MAXER.
 *      n = SHRT_MIN if checked by MINER.
 */
short minimax(PLAYER player, short depth, short alpha, short beta)
{
    TtableEntry &t_entry = t_table[hash % TABLE_SZ];
    if (t_entry.hash == hash)
        return t_entry.score;

    short score = 0, child_score = 0;
    bool has_move = false;
    BoardEntry *capture = nullptr;

    if (depth >= MAX_DEPTH)
        return approx(player) * (player ? 1 : -1);

    for (short ind = 0; ind <= BEGIN[player][KING]; ind++) // LVA: start with most worthless shapes
    {
        short sq_i = board[player][ind].sq;
        if (sq_i >= 0) // if not captured
        {
            SHAPE shape = board[player][ind].shape;
            for (short sq_f : gen_moves(player, shape, sq_i))
            {
                capture = move(player, shape, sq_i, sq_f);

                if (!is_repeat3(depth) && !is_attacked(player, board[player][BEGIN[player][KING]].sq))
                {
                    has_move = true;
                    m_table[depth] = hash;
                    child_score = minimax(PLAYER(!player), depth+1, alpha, beta);

                    if (player)
                        alpha = std::max(alpha, child_score);
                    else
                        beta = std::min(beta, child_score);

                    if (beta <= alpha)
                    {
                        move(player, shape, sq_f, sq_i, capture); // undo
                        score = (player) ? alpha : beta;
                        if (phase <= t_entry.phase) // if many hash have the same index, keep the one closer to endgame
                            t_entry = {hash, score, phase};
                        return score;
                    }
                }
                move(player, shape, sq_f, sq_i, capture); // undo
            }
        }
    }

    if (has_move)
        score = (player) ? alpha : beta;

    else if (is_attacked(player, board[player][BEGIN[player][KING]].sq)) // checkmated
        score = (player) ? SHRT_MIN : SHRT_MAX;
    
    if (phase <= t_entry.phase)
        t_entry = {hash, score, phase};
    return score;
}

/**
 * Behave the same as minimax(), except with recording the best move and without recording to t_table and AlphaBeta Prunning.
 * @return 0: bot has move(s). 1: bot is checkmated. 2: bot is stalemated.
 */
short bot_move(PLAYER player)
{
    short child_score = 0, best_sq_i = 0, best_sq_f = 0, best_score = (player) ? SHRT_MIN : SHRT_MAX;
    SHAPE best_shape;
    bool has_move = false;
    BoardEntry *capture = nullptr;

    for (short ind = 0; ind <= BEGIN[player][KING]; ind++)
    {
        short sq_i = board[player][ind].sq;
        if (sq_i >= 0) // if not captured
        {
            SHAPE shape = board[player][ind].shape;
            std::cout << "Possible {move, score} from " << sq_i << ": ";
            for (short sq_f : gen_moves(player, shape, sq_i))
            {
                capture = move(player, shape, sq_i, sq_f);
                
                if (!is_attacked(player, board[player][BEGIN[player][KING]].sq))
                {
                    has_move = true;
                    child_score = minimax(PLAYER(!player), 0, SHRT_MIN, SHRT_MAX);
                
                    std::cout << "{" << sq_f << ", " << child_score << "}, ";
                    if (player)
                    {
                        if (child_score > best_score)
                        {
                            best_shape = shape;
                            best_sq_i = sq_i;
                            best_sq_f = sq_f;
                            best_score = child_score;
                        }
                    }
                    else if (child_score < best_score) // if MINER
                    {
                        best_shape = shape;
                        best_sq_i = sq_i;
                        best_sq_f = sq_f;
                        best_score = child_score;
                    }
                }
                move(player, shape, sq_f, sq_i, capture); // undo
            }
            std::cout << std::endl;
        }
    }
    if (has_move)
    {
        std::cout << "Chosen move: " << best_sq_i << " to " << best_sq_f << std::endl;
        move(player, best_shape, best_sq_i, best_sq_f);
        return 0;
    }
    else if (!has_move && is_attacked(player, board[player][BEGIN[player][KING]].sq)) // checkmated
        return 1;
    
    else
        return 2;
}

/**
 * @return whether move is valid.
 * 0: valid. 1: illegal move vector. 2: empty initial square. 3: impersonating bot. 4: outside play area, 5: friendly fire, 6: blocked path, 7: checked
 */
short validate(short sq_i, short sq_f)
{
    if (!squares[sq_i])
        return 2;

    SHAPE shape = (*squares[sq_i]).shape; 
    PLAYER player = (*squares[sq_i]).player;
    short dx = abs(x_of(sq_f) - x_of(sq_i)),
          dy = abs(y_of(sq_f) - y_of(sq_i));

    if (player == BOT)
        return 3;
    
    if (!is_play_area(sq_f))
        return 4;

    if (squares[sq_f] && !can_capture(player, (*squares[sq_f]).player, (*squares[sq_f]).shape))
        return 5;

    if (shape == KING && std::max(dx, dy) > 1)
        return 1;

    if (shape == KNIGHT && (dx != 1 || dy != 2) && (dx != 2 || dy != 1))
        return 1;

    if (shape == QUEEN && dx && dy && dx != dy)
        return 1; 

    if (shape == ROOK && dx && dy)
        return 1;
    
    if (shape == BISHOP && dx != dy)
        return 1;

    if (shape == PAWN)
    {
        if (rel_foward(player)*(sq_f - sq_i) < 0) // moved backward
            return 1;

        if (!squares[sq_f])
        {
            if (dx != 0)
                return 1;

            if (dy > 1 + (y_of(sq_i) == 1 || y_of(sq_i) == 6)) // if pawn at starting line: dy > 1+(1); else: dy>1+(0)
                return 1;
        }
        else if (dx != 1 || dy != 1) // && not empty
            return 1;
    }

    if (shape != KING && shape != KNIGHT && !is_path_clear(sq_i, sq_f, dx, dy))
        return 6;

    BoardEntry *capture = move(player, shape, sq_i, sq_f);
    if (is_attacked(player, board[player][BEGIN[player][KING]].sq))
    {
        move(player, shape, sq_f, sq_i, capture); // undo
        return 7;
    }
    move(player, shape, sq_f, sq_i, capture); // undo

    // if all valid
    return 0;
}

void console_play()
{
    short sq_i = 0, sq_f = 0, invalid = 0;
    std::vector<short> inds_f;

    while (true)
    {
        out_board(true, true, true, true, true, true);
        do
        {
            std::cout << "Initial and final entry (intitial square, final square): ";
            std::cin >> sq_i >> sq_f;
            invalid = validate(sq_i, sq_f);
            if (invalid)
                std::cout << "Invalid move, broken rule #" << invalid << std::endl;
        }
        while (invalid);
        move(HUMAN, (*squares[sq_i]).shape, sq_i, sq_f);
        std::cout << std::endl;

        short outcome = bot_move(BOT);
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
