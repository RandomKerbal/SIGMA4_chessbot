#include <climits>
#include <random>

// remove in VEXIQ
#include <iostream>
#include <iomanip>

std::mt19937_64 rng(0);

const short MAX_DEPTH = 8;
const short NM_R = 3;
const short NM_DEPTH_INC = NM_R + 1;
const short MAX_NM_DEPTH = MAX_DEPTH - NM_DEPTH_INC;
const short MAX_NUM_CHILD = 54*3; // must be multiple of 3

enum PLAYER: short {
    BLACK = 0, WHITE = 1,
    MAX_PLAYER = 2,
    MINER = BLACK, MAXER = WHITE,
    BOT = BLACK, HUMAN = WHITE,
};
inline PLAYER operator!(PLAYER player)
{
    return PLAYER(player^1);
}
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
    PLAYER player;
    SHAPE shape;
    short sq = -1;
};
/**
 * board
 * └── 0,1: see enum PLAYER
 *     └── 0...16: max total number of pieces of each player
 *         └── entry: { player, shape, sq }
 * 
 * IMPORTANT: Shape order of entries must match enum SHAPE! 
 * If an entry is captured, its sq = sq - AREA. If an entry is empty, its sq = -1.
 */
BoardEntry board[MAX_PLAYER][16] = {
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
unsigned long long Z_IS_BLACK = 0;
unsigned long long hash = 0;

const unsigned int TABLE_SZ = 1 << 22; // must be power of 2
struct TtableEntry {
    unsigned long long hash = 0;
    short score = 0;
    short phase = SHRT_MAX;
};
TtableEntry t_table[TABLE_SZ]; // Transposition Table
unsigned long long hash_history[MAX_DEPTH] = {0};

float MAX_PHASE = 0; // sum of SHAPE_PHASE of all initial pieces
short phase = 0; // sum of SHAPE_PHASE of all current pieces
short worth_opening[MAX_PLAYER] = {0, 0};
short worth_endgame[MAX_PLAYER] = {0, 0};

/**
 * WORTH_OPENING/ENDGAME (aka Piece-Square Table)
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
 * With BLACK at the top and WHITE at the bottom, black's foward = +WIDTH, white's foward = -WIDTH.
 */
short rel_foward[MAX_PLAYER] = { WIDTH, -WIDTH };

inline bool is_play_area(short sq)
{
    return unsigned(sq) < AREA && x_of(sq) < 8; // if x < 0, it becomes a huge unsigned number > AREA.
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
        // BEGIN, squares, phase, worth_opening, worth_endgame, MAX_PHASE
        SHAPE prev_shape = PAWN, shape;
        short i = 0;
        for (BoardEntry &entry : board[player])
        {
            short sq = entry.sq;
            if (sq >= 0)
            {
                shape = entry.shape;
                if (shape > prev_shape)
                    BEGIN[player][shape] = i;

                squares[sq] = &entry;
                phase += SHAPE_PHASE[shape];
                worth_opening[player] += WORTH_OPENING[player][shape][sq];
                worth_endgame[player] += WORTH_ENDGAME[player][shape][sq];
                prev_shape = shape;
                i++;
            }
        }
        MAX_PHASE = phase;

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
    Z_IS_BLACK = rng();

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

inline unsigned long long move(unsigned long long hash, PLAYER player, SHAPE shape, short sq_i, short sq_f)
{
    BoardEntry * &sq_f_ptr = squares[sq_f];
    if (sq_f_ptr)
    {
        BoardEntry &del_entry = *sq_f_ptr;
        del_entry.sq -= AREA; // move sq outside board

        PLAYER del_player = del_entry.player;
        SHAPE del_shape = del_entry.shape;
        phase -= SHAPE_PHASE[del_shape];
        worth_opening[del_player] -= WORTH_OPENING[del_player][del_shape][sq_f];
        worth_endgame[del_player] -= WORTH_ENDGAME[del_player][del_shape][sq_f];
        hash ^= Ztable[del_player][del_shape][sq_f];
    }

    BoardEntry * &sq_i_ptr = squares[sq_i];

    // add to final square
    (*sq_i_ptr).sq = sq_f;
    sq_f_ptr = sq_i_ptr;
    worth_opening[player] += WORTH_OPENING[player][shape][sq_f];
    worth_endgame[player] += WORTH_ENDGAME[player][shape][sq_f];
    hash ^= Ztable[player][shape][sq_f];
    
    // remove from initial square
    sq_i_ptr = nullptr;
    worth_opening[player] -= WORTH_OPENING[player][shape][sq_i];
    worth_endgame[player] -= WORTH_ENDGAME[player][shape][sq_i];
    hash ^= Ztable[player][shape][sq_i];

    hash ^= Z_IS_BLACK;
    return hash;
}

/**
 * IMPORTANT:
 * 1. Doesn't undo hash.
 * 2. Same sq_i & sq_f as doing move.
 */
inline void unmove(PLAYER player, SHAPE shape, short sq_i, short sq_f, BoardEntry *add_ptr)
{
    if (add_ptr)
    {
        BoardEntry &add_entry = *add_ptr;
        add_entry.sq += AREA; // move sq back inside

        PLAYER add_player = add_entry.player;
        SHAPE add_shape = add_entry.shape;
        phase += SHAPE_PHASE[add_shape];
        worth_opening[add_player] += WORTH_OPENING[add_player][add_shape][sq_f];
        worth_endgame[add_player] += WORTH_ENDGAME[add_player][add_shape][sq_f];
    }

    BoardEntry * &sq_f_ptr = squares[sq_f];

    // add to initial square
    (*sq_f_ptr).sq = sq_i;
    squares[sq_i] = sq_f_ptr;
    worth_opening[player] += WORTH_OPENING[player][shape][sq_i];
    worth_endgame[player] += WORTH_ENDGAME[player][shape][sq_i];
    
    // remove from final square
    sq_f_ptr = add_ptr;
    worth_opening[player] -= WORTH_OPENING[player][shape][sq_f];
    worth_endgame[player] -= WORTH_ENDGAME[player][shape][sq_f];
}

/**
 * @return whether the given victim (_V) is the enemy of the given player && not king.
 */
inline bool can_capture(PLAYER player, PLAYER player_v, SHAPE shape_v)
{
    return player_v != player && shape_v != KING;
}

class MVVLVAMoveGenerator
{
    public:
        MVVLVAMoveGenerator(PLAYER player)
        {
            gen_MVVLmoves_va(player);
        }

        /**
         * @return whether moves[] has element to assign to sq_i & sq_f.
         */
        bool next(SHAPE &shape, short &sq_i, short &sq_f)
        {
            while (i_v < KING)
            {
                while (i_a < MAX_SHAPE)
                {
                    short *arr = moves[i_v][i_a];

                    if (ii + 2 < moves_sz[i_v][i_a])
                    {
                        shape = SHAPE(arr[ii++]);
                        sq_i = arr[ii++];
                        sq_f = arr[ii++];
                        return true;
                    }
                    ii = 0;
                    i_a++;
                }
                i_a = 0;
                i_v++;
            }
            return false;
        };

    private:
        /**
         * moves
         * └── victims ordered from most to least valuable: QUEEN, ..., PAWN (KING cannot be captured)
         *     └── attackers ordered from least to most valuable: PAWN, ..., KING
         *         └── shape, sq_i, sq_f, shape, sq_i, sq_f, ...
         * 
         * quiet_moves[] is the last array in moves[] (KING x PAWN).
         */
        short moves[KING][MAX_SHAPE][MAX_NUM_CHILD] = {{{}}}, moves_sz[KING][MAX_SHAPE] = {{}}, *quiet_moves = moves[QUEEN][KING], &quiet_moves_sz = moves_sz[QUEEN][KING];
        short sq_i = 0, sq_f = 0, i_v = 0, i_a = 0, ii = 0;
        SHAPE shape_a, shape_v;
        BoardEntry victim;

        inline void MVVLVA_insert()
        {
            short *moves_va = moves[QUEEN-shape_v][shape_a];
            short &moves_sz_va = moves_sz[QUEEN-shape_v][shape_a];
            moves_va[moves_sz_va++] = shape_a;
            moves_va[moves_sz_va++] = sq_i;
            moves_va[moves_sz_va++] = sq_f;
        }

        inline void quiet_insert()
        {
            quiet_moves[quiet_moves_sz++] = shape_a;
            quiet_moves[quiet_moves_sz++] = sq_i;
            quiet_moves[quiet_moves_sz++] = sq_f;
        }

        void gen_MVVLmoves_va(PLAYER player)
        {
            for (short i = 0; i <= BEGIN[player][KING]; i++)
            {
                sq_i = board[player][i].sq;
                if (sq_i >= 0) // if not captured
                {
                    shape_a = board[player][i].shape;
                    if (shape_a == PAWN)
                    {
                        short y_i = y_of(sq_i);
                        if (1 <= y_i && y_i <= 6)
                        {
                            short dy = rel_foward[player], sq_y = sq_i + dy;

                            // capture moves
                            for (short dx = -1; dx <= 1; dx += 2)
                            {
                                sq_f = sq_y + dx;
                                if (sq_f >= 0 && squares[sq_f])
                                {
                                    victim = *squares[sq_f];
                                    shape_v = victim.shape;
                                    if (can_capture(player, victim.player, shape_v))
                                        MVVLVA_insert();
                                }
                            }

                            // y-moves
                            sq_f = sq_y;
                            if (!squares[sq_f])
                            {
                                quiet_insert();
                                if ((y_i == 1 && dy > 0) || (y_i == 6 && dy < 0)) // can step 2
                                {
                                    sq_f += dy;
                                    if (!squares[sq_f])
                                        quiet_insert();
                                }
                            }
                        }
                    }
                    else if (shape_a == KNIGHT)
                    {
                        for (short v: N_VECTOR)
                        {
                            sq_f = sq_i + v;
                            if (is_play_area(sq_f))
                            {
                                if (!squares[sq_f])
                                    quiet_insert();
                                else
                                {
                                    victim = *squares[sq_f];
                                    shape_v = victim.shape;
                                    if (can_capture(player, victim.player, shape_v))
                                        MVVLVA_insert();
                                }
                            }
                        }
                    }
                    else if (shape_a == KING)
                    {
                        for (short v: K_VECTOR)
                        {
                            sq_f = sq_i + v;
                            if (is_play_area(sq_f))
                            {
                                if (!squares[sq_f])
                                    quiet_insert();
                                else
                                {
                                    victim = *squares[sq_f];
                                    shape_v = victim.shape;
                                    if (can_capture(player, victim.player, shape_v))
                                        MVVLVA_insert();
                                }
                            }
                        }
                    }
                    else
                    {
                        if (shape_a == BISHOP || shape_a == QUEEN)
                        {
                            for (short v: B_VECTOR)
                            {
                                for (sq_f = sq_i + v; is_play_area(sq_f) && !squares[sq_f]; sq_f += v)
                                    quiet_insert();
                                
                                if (is_play_area(sq_f))
                                {
                                    victim = *squares[sq_f];
                                    shape_v = victim.shape;
                                    if (can_capture(player, victim.player, shape_v))
                                        MVVLVA_insert();
                                }
                            }
                        }
                        if (shape_a == ROOK || shape_a == QUEEN)
                        {
                            for (short v: R_VECTOR)
                            {
                                // travel until on top of a shape
                                for (sq_f = sq_i + v; is_play_area(sq_f) && !squares[sq_f]; sq_f += v)
                                    quiet_insert();

                                if (is_play_area(sq_f))
                                {
                                    victim = *squares[sq_f];
                                    shape_v = victim.shape;
                                    if (can_capture(player, victim.player, shape_v))
                                        MVVLVA_insert();
                                }
                            }
                        }
                    }
                }
            }
        }
};

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
 * @return whether the given square is currently attacked by the enemy of the given player.
 */
bool is_checked(PLAYER player)
{
    short i = 0, dx = 0, dy = 0, y = 0, sq_a = 0, sq_v = board[player][BEGIN[player][KING]].sq;

    // check for enemy pawns by being a pawn and check where I can capture
    y = y_of(sq_v);
    if (1 <= y && y <= 6)
    {
        short sq_y = sq_v + rel_foward[player];
        BoardEntry attacker;
        for (dx = -1; dx <= 1; dx += 2)
        {
            sq_a = sq_y + dx;
            if (sq_a >= 0 && squares[sq_a])
            {
                attacker = *squares[sq_a];
                if (attacker.player != player && attacker.shape == PAWN)
                    return true;
            }
        }
    }

    // check for enemy knights by iterating over all enemy knights
    for (i = BEGIN[!player][KNIGHT]; i < BEGIN[!player][BISHOP]; i++)
    {
        sq_a = board[!player][i].sq;
        if (sq_a >= 0) // if not captured
        {
            dx = abs(x_of(sq_a) - x_of(sq_v));
            dy = abs(y_of(sq_a) - y_of(sq_v));
            if ((dx == 1 && dy == 2) || (dx == 2 && dy == 1))
                return true;
        }
    }

    // check for enemy bishops
    for (i = BEGIN[!player][BISHOP]; i < BEGIN[!player][ROOK]; i++)
    {
        sq_a = board[!player][i].sq;
        if (sq_a >= 0)
        {
            dx = abs(x_of(sq_a) - x_of(sq_v));
            dy = abs(y_of(sq_a) - y_of(sq_v));
            if (dx == dy && is_path_clear(sq_v, sq_a, dx, dy))
                return true;
        }
    }

    // check for enemy rooks
    for (i = BEGIN[!player][ROOK]; i < BEGIN[!player][QUEEN]; i++)
    {
        sq_a = board[!player][i].sq;
        if (sq_a >= 0)
        {
            dx = abs(x_of(sq_a) - x_of(sq_v));
            dy = abs(y_of(sq_a) - y_of(sq_v));
            if ((dx == 0 || dy == 0) && is_path_clear(sq_v, sq_a, dx, dy))
                return true;
        }
    }

    // check for enemy queen
    sq_a = board[!player][BEGIN[!player][QUEEN]].sq;
    if (sq_a >= 0)
    {
        dx = abs(x_of(sq_a) - x_of(sq_v));
        dy = abs(y_of(sq_a) - y_of(sq_v));
        if ((dx == dy || dx == 0 || dy == 0) && is_path_clear(sq_v, sq_a, dx, dy))
            return true;
    }

    // no check for enemy king since king cannot attack king

    return false;
}

void out_board(bool has_t_table = false, bool has_hash = false, bool has_index = false, bool has_phase = false, bool has_worth = false)
{
    const char char_of[MAX_PLAYER][MAX_SHAPE] = {
        { 'p', 'n', 'b', 'r', 'q', 'k' },
        { 'P', 'N', 'B', 'R', 'Q', 'K' },
    };
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
            for (short i = 0; i <= BEGIN[player][KING]; i++)
            {
                BoardEntry &entry = board[player][i];
                std::cout << '[' << char_of[player][entry.shape] << "]=";
                std::cout << entry.sq << ", ";
            }
        }
        std::cout << std::endl;
    }

    if (has_phase)
        std::cout << "Distance from endgame:" << std::endl << TAB << phase << '/' << MAX_PHASE << std::endl;

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

    std::cout << TAB << "+------------BLACK------------+" << std::endl;
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
    std::cout << TAB << "+------------WHITE------------+" << std::endl;
}

inline short static_eval()
{
    short score_opening = worth_opening[MAXER] - worth_opening[MINER],
          score_endgame = worth_endgame[MAXER] - worth_endgame[MINER];

    return (score_endgame - (phase/MAX_PHASE)*(score_endgame - score_opening)); // linear interpolation
}

inline bool is_repeat(unsigned long long hash, short depth)
{
    if (depth >= 4 && hash == hash_history[depth-4])
        return true;
    else
        return false;
}

inline void into_t_table(TtableEntry &t_entry, unsigned long long hash, short score)
{
    if (phase <= t_entry.phase) // if hash collision, keep the one closer to endgame
    {
        t_entry.hash = hash;
        t_entry.score = score;
        t_entry.phase = phase;
    }
}

/**
 * Minimax + Tapered Piece-Square Table Evaluation + AlphaBeta Pruning + Null-Move Prunning + Zobrist Hashing Transposition Table + MVV-LVA + Threefold Repetition Check
 * In first call, use depth = 1, alpha = SHRT_MIN, beta = SHRT_MAX.
 * @return
 *      n ∈ (SHRT_MIN, 0) U (0, SHRT_MAX) if over MAX_DEPTH.
 *      n = SHRT_MAX if check/stalemated by MAXER.
 *      n = SHRT_MIN if check/stalemated by MINER.
 */
short eval(unsigned long long hash, PLAYER player, short depth, short alpha, short beta, bool is_NM_eval, bool is_PV_node)
{
    TtableEntry &t_entry = t_table[hash % TABLE_SZ];
    if (hash == t_entry.hash)
        return t_entry.score;

    if (depth == MAX_DEPTH)
        return static_eval();

    hash_history[depth] = hash;

    // Null-Move (NM) Pruning
    short child_score = 0;
    if (!is_NM_eval && !is_PV_node && depth <= MAX_NM_DEPTH && phase > 0 && !is_checked(player))
    {
        if (player == MAXER)
        {
            child_score = eval(hash^Z_IS_BLACK, !player, depth + NM_DEPTH_INC, beta-1, beta, true, is_PV_node);
            if (child_score >= beta)
            {
                hash_history[depth] = 0;
                return beta;
            }
        }
        else
        {
            child_score = eval(hash^Z_IS_BLACK, !player, depth + NM_DEPTH_INC, alpha, alpha+1, true, is_PV_node);
            if (child_score <= alpha)
            {
                hash_history[depth] = 0;
                return alpha;
            }
        }
    }

    short score = (player == MAXER) ? SHRT_MIN : SHRT_MAX, sq_i = 0, sq_f = 0;
    unsigned long long child_hash = 0;
    SHAPE shape;
    BoardEntry *sq_f_ptr;

    MVVLVAMoveGenerator Moves(player);
    while (Moves.next(shape, sq_i, sq_f))
    {
        sq_f_ptr = squares[sq_f];
        child_hash = move(hash, player, shape, sq_i, sq_f);

        if (!is_repeat(child_hash, depth) && !is_checked(player))
        {
            child_score = eval(child_hash, !player, depth+1, alpha, beta, is_NM_eval, is_PV_node);
            unmove(player, shape, sq_i, sq_f, sq_f_ptr);
            is_PV_node = false;
            
            if (player == MAXER)
            {
                if (child_score > score)
                    score = child_score;
                if (child_score > alpha)
                    alpha = child_score;
            }
            else
            {
                if (child_score < score)
                    score = child_score;
                if (child_score < beta)
                    beta = child_score;
            }
            if (alpha >= beta)
            {
                into_t_table(t_entry, hash, score);
                hash_history[depth] = 0;
                return score;
            }
        }
        else
            unmove(player, shape, sq_i, sq_f, sq_f_ptr);
    }
    into_t_table(t_entry, hash, score);
    hash_history[depth] = 0;
    return score;
}

/**
 * Similar to eval().
 * @return 0: bot has move(s). 1: bot is checkmated. 2: bot is stalemated.
 */
short bot_move(PLAYER player)
{
    bool is_PV_node = true;
    short child_score = 0, best_sq_i = 0, best_sq_f = 0, score = (player == MAXER) ? SHRT_MIN : SHRT_MAX, sq_i = 0, sq_f = 0;
    unsigned long long child_hash = 0;
    SHAPE best_shape, shape;
    BoardEntry *sq_f_ptr;

    hash_history[0] = hash;

    std::cout << "Possible {square_i, square_f, score}:" << std::endl;
    MVVLVAMoveGenerator Moves(player);
    while (Moves.next(shape, sq_i, sq_f))
    {
        sq_f_ptr = squares[sq_f];
        child_hash = move(hash, player, shape, sq_i, sq_f);
        
        if (!is_checked(player))
        {
            child_score = eval(child_hash, !player, 1, SHRT_MIN, SHRT_MAX, false, is_PV_node);
            unmove(player, shape, sq_i, sq_f, sq_f_ptr);
            is_PV_node = false;
            std::cout << "{" << sq_i << ", " << sq_f << ", " << child_score << "}, " << std::endl;
        
            if (player == MAXER && child_score > score)
            {
                best_shape = shape;
                best_sq_i = sq_i;
                best_sq_f = sq_f;
                score = child_score;
            }
            else if (player == MINER && child_score < score)
            {
                best_shape = shape;
                best_sq_i = sq_i;
                best_sq_f = sq_f;
                score = child_score;
            }
        }
        else
            unmove(player, shape, sq_i, sq_f, sq_f_ptr);
    }
    std::cout << std::endl;
    if (score != SHRT_MIN && score != SHRT_MAX)
    {
        std::cout << "Chosen move: " << best_sq_i << " to " << best_sq_f << std::endl;
        hash = move(hash, player, best_shape, best_sq_i, best_sq_f);
        return 0;
    }
    else if (is_checked(player)) // checkmated
        return 1;

    else
        return 2;
}

/**
 * @return whether move is valid.
 * 0: valid. 1: illegal move vector. 2: empty initial square. 3: outside play area. 4: impersonating bot. 5: illegal capture, 6: blocked path, 7: checked
 */
short validate(short sq_i, short sq_f)
{
    BoardEntry *sq_i_ptr = squares[sq_i], *sq_f_ptr = squares[sq_f];
    if (!sq_i_ptr)
        return 2;

    if (!is_play_area(sq_i) || !is_play_area(sq_f))
        return 3;

    PLAYER player = (*sq_i_ptr).player;
    SHAPE shape = (*sq_i_ptr).shape;
    short dx = abs(x_of(sq_f) - x_of(sq_i)),
          dy = abs(y_of(sq_f) - y_of(sq_i));

    if (player == BOT)
        return 4;

    if (sq_f_ptr && !can_capture(player, (*sq_f_ptr).player, (*sq_f_ptr).shape))
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
        if (rel_foward[player]*(sq_f - sq_i) < 0) // moved backward
            return 1;

        if (!sq_f_ptr)
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

    
    // check checked
    move(hash, player, shape, sq_i, sq_f);
    if (is_checked(player))
    {
        unmove(player, shape, sq_i, sq_f, sq_f_ptr);
        return 7;
    }
    unmove(player, shape, sq_i, sq_f, sq_f_ptr);

    // if all valid
    return 0;
}

void console_play()
{
    short sq_i = 0, sq_f = 0, invalid = 0;
    while (true)
    {
        out_board(true, true, true, true, true);
        do
        {
            std::cout << "Initial and final entry (intitial square, final square): ";
            std::cin >> sq_i >> sq_f;
            invalid = validate(sq_i, sq_f);
            if (invalid)
            {
                if (invalid == 7)
                {
                    std::cout << "You lost!" << std::endl;
                    exit(0);
                }
                else
                    std::cout << "Invalid move, broken rule #" << invalid << std::endl;
            }
        }
        while (invalid);
        move(hash, HUMAN, (*squares[sq_i]).shape, sq_i, sq_f);
        std::cout << std::endl;

        short outcome = bot_move(BOT);
        if (outcome == 1)
            std::cout << "You won!" << std::endl;
        else if (outcome == 2)
            std::cout << "Ended in draw." << std::endl;
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
