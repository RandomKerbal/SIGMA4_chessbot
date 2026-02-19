#include <cmath>
#include <cctype>
#include <vector>
#include <climits>
#include <algorithm>

// remove in VEXIQ
#include <iostream>
#include <iomanip>

#define shape_of std::tolower

const int PLAY_SIZE = 8; // playable size
const int SIZE = PLAY_SIZE + 2; // playable size + sentinels
const int AREA = PLAY_SIZE*SIZE;
/**
 * IMPORTANT: the board is 10x8 since the two rightmost columns are sentinels
 *            that prevent pointers from "wrapping" onto the previous/next row.
 *            The playable area is the leftmost 8x8.
 * 
 * Sentinel/empty are 0 || '\0' to identify easily using !board[ind]
 * 
 * board
 * ├── columns 8,9 element: sentinel
 * └── columns 0-8 element (aka tile):
 *     ├── 0: empty
 *     ├── lowercase: black, uppercase: white
 *     └── k/K: king, q/Q: queen, r/R: rook, b/B: bishop, n/N: knight, p/P: pawn
 */
char board[AREA] = {
    'r','n','b','q','k','b','n','r', 0, 0,
    'p','p','p','p','p','p','p','p', 0, 0,
     0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0, 0,
     0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0, 0,
     0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0, 0,
     0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0, 0,
    'P','P','P','P','P','P','P','P', 0, 0,
    'R','N','B','Q','K','B','N','R', 0, 0,
};

const int SHAPE_I = 'b'; // 98
const int SHAPE_F = 'r'; // 114
const int SHAPE_MAX = SHAPE_F - SHAPE_I + 1;
const int TILE_MAX = 8;
/**
 * IMPORTANT: Must call init_tiles() at the start of every program! It starts as empty.
tiles
├── 0: black
│   ├── k-b || 107-98: black king index
│   ├── n-b || 110-98: black knight indices
│   ├── q-b || 113-98: black queen index
│   ├── r-b || 114-98: black rook indices
│   └── b-b || 98-98 : black bishop indices
│
└── 1: white
    ├── k-b || 107-98 : white king index
    ├── n-b || 110-98 : white knight indices
    ├── q-b || 113-98 : white queen index
    ├── r-b || 114-98 : white rook indices
    └── b-b || 98-98 : white bishop indices

If a tile is captured, its index becomes -1.
 */
int tiles[2][SHAPE_MAX][TILE_MAX];

/**
 * tile_count
 * ├── 0: number of tiles occupied by black
 * └── 1: number of tiles occupied by white
 */
int tile_count[2] = {16, 16};

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
    +SIZE,      // down
    -SIZE       // up
};
const int B_VECTOR[4] = {
    +1 + SIZE,  // down-right
    +1 - SIZE,  // up-right
    -1 + SIZE,  // down-left
    -1 - SIZE   // up-left
};
// no Q_VECTOR since it's just a combination of R_VECTOR & B_VECTOR

bool MAXER = 1, // white
    MINER = 0; // black
const int MAX_DEPTH = 5;

/**
 * IMPORTANT: Before calling, use board[ind] to check tile is NOT empty since 0 can = black or empty!
 * @return the player occupying the given tile.
 * 0: black player || empty, 1: white player
 */
inline bool player_of(char tile)
{
    return tile < 'a'; // uppercase letters are ASCII < 'a'
}

void init_tiles()
{
    // initialize all to -1
    for (int (&player)[SHAPE_MAX][TILE_MAX] : tiles)
    {
        for (int (&shape)[TILE_MAX] : player)
        {
            for (int &ind : shape)
                ind = -1;
        }
    }

    for (int ind = 0; ind < AREA; ind++)
    {
        char tile = board[ind];
        if (tile)
        {
            bool player = player_of(tile);
            char shape = shape_of(tile);
            int *begin = tiles[player][shape-SHAPE_I];
            int *end   = begin + TILE_MAX;
            *std::find(begin, end, -1) = ind;
        }
    }
}

/**
 * @return the forward direction relative to the given player.
 * In the configuration where white at bottom and black at top, white's foward = -1, black's foward = +1.
 */
inline int rel_foward(bool player)
{
    return (player) ? -1 : 1;
}

inline int xy_of(int ind, bool is_y)
{
    return (is_y) ? ind / SIZE : ind % SIZE;
}

/**
 * @return captured tile.
 */
inline char move(bool player, int ind_i, int ind_f, char restore = 0)
/// TODO: pawn promotion
{
    int *begin = tiles[player][shape_of(board[ind_i])];
    int *end = begin + TILE_MAX;
    *std::find(begin, end, ind_i) = ind_f;

    char capture = board[ind_f];
    if (capture)
    {
        tile_count[!player]--;
        int *begin = tiles[!player][shape_of(board[ind_f])];
        int *end = begin + TILE_MAX;
        *std::find(begin, end, ind_f) = -1;
    }
    else if (restore)
    {
        tile_count[!player] ++;
        int *begin = tiles[!player][shape_of(restore)];
        int *end = begin + TILE_MAX;
        *std::find(begin, end, ind_f) = ind_i;
    }

    board[ind_f] = board[ind_i];
    board[ind_i] = restore;
    return capture;
}

/**
 * IMPORTANT: Before calling, use board[ind] to check tile is NOT empty since 0 can = black or empty!
 * @return whether the given tile has the enemy of the given player && not king.
 */
inline bool can_capture(bool player, char tile)
{
    return player_of(tile) != player && shape_of(tile) != 'k';
}

inline bool is_play_area(int ind)
{
    return xy_of(ind, 0) < 8 && 0 <= ind && ind < AREA;
}

std::vector<int> gen_moves(bool player, int ind_i)
{
    std::vector<int> moves;
    int ind = 0;
    char shape = shape_of(board[ind_i]), tile = 0;
    
    if (shape == 'k')
    {
        for (int v: K_VECTOR)
        {
            ind = ind_i + v;
            tile = board[ind];
            if (is_play_area(ind) && (!tile || can_capture(player, tile)))
                moves.push_back(ind);
        }
    }
    if (shape == 'n')
    {
        for (int v: N_VECTOR)
        {
            ind = ind_i + v;
            tile = board[ind];
            if (is_play_area(ind) && (!tile || can_capture(player, tile)))
                moves.push_back(ind);
        }
    }
    if (shape == 'r' || shape == 'q')
    {
        for (int v: R_VECTOR)
        {
            // travel until on top of a shape
            for (ind = ind_i + v; is_play_area(ind) && !board[ind]; ind += v)
                moves.push_back(ind);
            
            if (is_play_area(ind) && can_capture(player, board[ind]))
                moves.push_back(ind);

        }
    }
    if (shape == 'b' || shape == 'q')
    {
        for (int v: B_VECTOR)
        {
            for (ind = ind_i + v; is_play_area(ind) && !board[ind]; ind += v)
                moves.push_back(ind);
            
            if (is_play_area(ind) && can_capture(player, board[ind]))
                moves.push_back(ind);
        }
    }
    if (shape == 'p')
    {
        ind_i += rel_foward(player)*SIZE;
        
        /// TODO: remove once added promotion
        if (ind_i < 0 || AREA <= ind_i)
            return moves;

        // capture moves
        for (int v: {-1, 1})
        {
            ind = ind_i + v;
            tile = board[ind];
            if (tile && can_capture(player, tile))
                moves.push_back(ind);
        }

        // y-moves
        if (!board[ind_i])  // y-check done at the top
        {
            moves.push_back(ind_i);
        
            if (xy_of(ind_i, 1) == 1 || xy_of(ind_i, 1) == 6) // if pawn can step 2
            {
                ind_i += rel_foward(player)*SIZE;
                if (!board[ind_i])
                    moves.push_back(ind_i);
            }
        }
    }

    return moves;
}

/**
 * @return whether the given tile is currently attacked by the enemy of the given player.
 */
bool is_attacked(bool player, int ind_i)
{
    int ind = 0, dx = 0, dy = 0;
    char tile = 0;

    // check for enemy king (start with less costly ones)
    ind = tiles[!player]['k'-SHAPE_I][0];
    dx = abs(xy_of(ind, 0) - xy_of(ind_i, 0));
    dy = abs(xy_of(ind, 1) - xy_of(ind_i, 1));
    if (std::max(dx, dy) == 1)
        return 1;

    // check for enemy knight
    for (int ind : tiles[!player]['n'-SHAPE_I])
    {
        dx = abs(xy_of(ind, 0) - xy_of(ind_i, 0));
        dy = abs(xy_of(ind, 1) - xy_of(ind_i, 1));
        if ((dx == 1 && dy == 2) && (dx == 2 && dy == 1))
            return 1;
    }

    /*
    Attacks (except pawn) are reversible. If a shape A' on tile A attacks tile B, then A' on tile B attacks tile A.
    Using this theorem,
    checking if shape A' on A attacks king on K = checking if A' on K attacks on A.
    */
    // let A' be enemy pawn
    ind = ind_i + rel_foward(player)*SIZE;
    if (0 <= ind && ind < AREA)
    {
        for (int v: {-1, 1})
        {
            tile = board[ind + v];
            if (tile && player_of(tile) != player && shape_of(tile) == 'p')
                return 1;
        }
    }
    // let A' be enemy rook or queen.
    for (int v: R_VECTOR)
    {
        for (ind = ind_i + v; is_play_area(ind) && !board[ind]; ind += v)
        {}
        tile = board[ind];
        if (tile && player_of(tile) != player && (shape_of(tile) == 'r' || shape_of(tile) == 'q'))
            return 1;
    }
    // let A' be enemy bishop or queen.
    for (int v: B_VECTOR)
    {
        for (ind = ind_i + v; is_play_area(ind) && !board[ind]; ind += v)
        {}
        tile = board[ind];
        if (tile && player_of(tile) != player && (shape_of(tile) == 'b' || shape_of(tile) == 'q'))
            return 1;
    }

    return 0;
}

void out_board()
{
    for (bool player : {0, 1})
    {
        std::cout << "Player" << player << " tiles: " << std::endl;
        for (char shape = SHAPE_I; shape <= SHAPE_F; shape++)
        {
            std::cout << '[' << shape << "]=";
            for (int ind : tiles[player][shape-SHAPE_I])
            {
                if (ind != -1)
                {
                    std::cout << ind << ",";
                }
            }
            std::cout << "; ";
        }
        std::cout << std::endl;

        std::cout << "Tile(s) player" << player << " attacks: ";
        for (int ind = 0; ind < AREA; ind++)
        {
            char tile = board[ind];
            if (tile && player_of(tile) != player && is_attacked(!player, ind))
                std::cout << ind << ", ";
        }
        std::cout << std::endl;
    }

    std::cout << "   +-------------BOT-------------+" << std::endl;
    for (int ind = 0; ind < AREA; ind++)
    {
        if (xy_of(ind, 0) > 7) // sentinels
        {
            std::cout << " . . | " << ind-1 << std::endl;
            ind++;
        }
        else
        {
            if (xy_of(ind, 0) == 0)
                std::cout << std::setw(2) << ind << " |";

            char tile = board[ind];
            std::cout << std::setw(3) << (tile ? tile : ' ');
        }
    }
    std::cout << "   +-------------YOU-------------+" << std::endl;
}

/**
 * Minimax + Depth-adjusted Score + AlphaBeta Prunning
 * In first call, use depth = 1, alpha = INT_MIN, beta = INT_MAX.
 * @return: -depth if stalemate. INT_MAX-depth if checked by MAXER. INT_MIN-depth if checked by MINER.
 */
int minimax(bool player, int depth, int alpha, int beta)

{
    int child_score = 0, tile_rem = tile_count[player];

    if (depth > MAX_DEPTH)
        return 0;

    for (int ind_i = 0; ind_i < AREA; ind_i++)
    {
        char tile_i = board[ind_i];
        if (tile_rem > 0 && tile_i && player_of(tile_i) == player)
        {
            tile_rem --;
            for (int ind_f : gen_moves(player, ind_i))
            {
                char capture = move(player, ind_i, ind_f);

                if (!is_attacked(player, tiles[player]['k'-SHAPE_I][0]))
                {
                    child_score = minimax(!player, depth+1, alpha, beta);

                    if (player == MAXER)
                        alpha = std::max(alpha, child_score);
                    else // if MINER
                        beta = std::min(beta, child_score);
                    if (beta <= alpha)
                    {
                        move(player, ind_f, ind_i, capture); // undo move
                        break;
                    }
                }
                move(player, ind_f, ind_i, capture); // undo move
            }
        }
    }

    if (child_score == 0) // if no legal move. If move exists, minimax() never return 0 and child_score != 0.
    {
        if (is_attacked(player, tiles[player]['k'-SHAPE_I][0])) // checkmate
            return (player == MAXER) ? INT_MIN + depth : INT_MAX - depth;
        
        // stalemate
        return -depth;
    }
    // if has legal move
    return (player == MAXER) ? alpha : beta;
}

void bot_move(bool bot)
{
    int best_ind_i = 0, best_ind_f = 0, best_score = 0, tile_rem = tile_count[bot];

    for (int ind_i = 0; ind_i < AREA; ind_i++)
    {
        char tile_i = board[ind_i];
        if (tile_rem > 0 && tile_i && player_of(tile_i) == bot)
        {
            tile_rem --;
            std::cout << "Possible {move, score} from " << ind_i << ": ";
            for (int ind_f : gen_moves(bot, ind_i))
            {
                char capture = move(bot, ind_i, ind_f);

                int score = minimax(!bot, 1, INT_MIN, INT_MAX);
                std::cout << "{" << ind_f << ", " << score << "}, ";
                if (bot == MAXER)
                {
                    if (score > best_score)
                    {
                        best_ind_i = ind_i;
                        best_ind_f = ind_f;
                        best_score = score;
                    }
                }
                else if (score < best_score) // if MINER
                {
                    best_ind_i = ind_i;
                    best_ind_f = ind_f;
                    best_score = score;
                }
                move(bot, ind_f, ind_i, capture); // undo move
            }
            std::cout << std::endl;
        }
    }
    std::cout << "Chosen move: " << best_ind_i << " to " << best_ind_f << std::endl;
    move(bot, best_ind_i, best_ind_f);
}

/**
 * @return whether move is valid.
 * 0: valid, 1: broken chess rule, 2: empty initial tile, 3: outside play area, 4: friendly fire, 5: blocked path
 * TODO: 5: impersonating bot, 6: checkmate, 7: stalemate
 */
int validate(int ind_i, int ind_f)
{
    int player = player_of(board[ind_i]), shape = shape_of(board[ind_i]),
        dx = abs(xy_of(ind_f, 0) - xy_of(ind_i, 0)), dy = abs(xy_of(ind_f, 1) - xy_of(ind_i, 1));

    if (!board[ind_i])
        return 2;

    if (!is_play_area(ind_f))
        return 3;

    if (board[ind_f] && !can_capture(player, board[ind_f]))
        return 4;

    // king && check direction
    if (shape == 'k' && std::max(dx, dy) > 1)
        return 1;

    // knight && check direction
    if ((shape == 'n') && (dx != 1 || dy != 2) && (dx != 2 || dy != 1))
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

    // pawn && check direction
    if (shape == 'p')
    {
        if (rel_foward(player) * (ind_f - ind_i) < 0) // moved backward
            return -1;

        if (!board[ind_f])
        {
            if (dx != 0)
                return 1;
            
            if (dy > 1 + (xy_of(ind_i, 1) == 1 || xy_of(ind_i, 1) == 6)) // if pawn at starting line: dy > 1+1; else: dy>1+0
                return 1;
        }
        else if (dx != 1 || dy != 1) // && not empty
            return 1;
    }

    if (shape != 'k' && shape != 'n')
    {
        // check if all tiles on the path are empty
        int v = (ind_f - ind_i) / std::max(dx, dy); 
        for (int ind = ind_i + v; ind != ind_f; ind += v)
        {
            if (board[ind])
                return 5;
        }
    }

    // if all valid
    return 0;
}

void console_play()
{
    int ind_i = 0, ind_f = 0, invalid = 0;
    std::vector<int> inds_f;

    while (true)
    {
        out_board();
        do
        {
            std::cout << "Initial and final tile (ind_i ind_f): ";
            std::cin >> ind_i >> ind_f;
            invalid = validate(ind_i, ind_f);
            if (invalid)
                std::cout << "Invalid move, broken rule #" << invalid << 0 << std::endl;
        }
        while (invalid);
        move(1, ind_i, ind_f);
        std::cout << std::endl;

        bot_move(0);
        std::cout << std::endl;
    }
}

int main()
{
    init_tiles();
    console_play();
    return 0;
}
