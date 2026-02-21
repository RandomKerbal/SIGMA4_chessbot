#include <cmath>
#include <vector>
#include <climits>
#include <algorithm>

// remove in VEXIQ
#include <iostream>
#include <iomanip>

/**
 *  IMPORTANT: SHAPE[0] is undefined. It starts at 1 to sync with shapes on board.
 *  Start with pieces more likely to capture others (pawns -> knights -> bishops -> rooks -> queens -> king).
 *  Pawns are first since capturing with a pawn is usually a good move -> enables more AlphaBeta prunning.
 */
enum SHAPE { PAWN = 1, KNIGHT = 2, BISHOP = 3, ROOK = 4, QUEEN = 5, KING = 6 };
enum PLAYER { BLACK = 0, WHITE = 1 };

const int PLAY_SIZE = 8, // playable width/height
    SIZE = PLAY_SIZE + 2, // playable width + sentinels
    AREA = PLAY_SIZE*SIZE;
/** 
 * board
 * ├── columns 8,9 element
 * │   └── 0: sentinel
 * │
 * └── columns 0...8 element (tile)
 *     ├── 0: empty
 *     ├── digit1 (player): see enum PLAYER
 *     └── digit2 (shape): see enum SHAPE
 * 
 * The board is 10x8 since the two rightmost columns are sentinels that prevent
 * pointers from "wrapping" onto the previous/next row. The real playable area
 * is the leftmost 8x8. Sentinel/empty are 0 to identify easily using !board[ind]
 */
int board[AREA] = {
     4,  2,  3,  5,  6,  3,  2,  4, 0, 0,
     1,  1,  1,  1,  1,  1,  1,  1, 0, 0,
     0,  0,  0,  0,  0,  0,  0,  0, 0, 0,
     0,  0,  0,  0,  0,  0,  0,  0, 0, 0,
     0,  0,  0,  0,  0,  0,  0,  0, 0, 0,
     0,  0,  0,  0,  0,  0,  0,  0, 0, 0,
    11, 11, 11, 11, 11, 11, 11, 11, 0, 0,
    14, 12, 13, 15, 16, 13, 12, 14, 0, 0,
};

const int MAX_SHAPE = 7; // maximum number of different shapes, starts at 1
const int MAX_NUM = 8;  // maximum number of pieces of any shape
const int NUM[MAX_SHAPE] = {0, MAX_NUM, 2, 2, 2, 1, 1}; // index = shape, val = number of pieces of the shape
/**
 * board2 (aka B2)
 * ├── 0 (black)
 * │   └── 1...6: see enum SHAPE
 * │       └── 0...NUM[MAX_SHAPE]-1: indexes on main board
 * │
 * └── 1 (white)
 *     └── 1...6: see enum SHAPE
 *         └── 0...NUM[MAX_SHAPE]-1: indexes on main board
 * 
 * IMPORTANT:
 * 1. Must call init_board2() at the start of every program!
 * 2. If a tile is captured, its index becomes index - AREA.
 */
int board2[2][MAX_SHAPE][MAX_NUM];

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
const int B_VECTOR[4] = {
    +1 + SIZE,  // down-right
    +1 - SIZE,  // up-right
    -1 + SIZE,  // down-left
    -1 - SIZE   // up-left
};
const int R_VECTOR[4] = {
    +1,         // right
    -1,         // left
    +SIZE,      // down
    -SIZE       // up
};
// no Q_VECTOR since it's a combination of B_VECTOR & R_VECTOR

const bool MAXER = 1, // white
    MINER = 0; // black
const int MAX_DEPTH = 5;

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
const char char_of[2][MAX_SHAPE] = {
    { ' ', 'p', 'n', 'b', 'r', 'q', 'k' },
    { ' ', 'P', 'N', 'B', 'R', 'Q', 'K' },
};

/**
 * IMPORTANT: Before calling, use board[ind] to check tile is NOT empty since return 0 can = black or empty!
 * @return the player occupying the given tile (see enum PLAYER).
 */
inline bool player_of(int tile)
{
    return tile/10; // extract digit1
}

inline int shape_of(int tile)
{
    return tile % 10; // extract digit2
}

void init_board2()
{
    // initialize all to -1
    for (bool player : {BLACK, WHITE})
    {
        for (int shape = 1; shape < MAX_SHAPE; shape++)
        {
            for (int ind = 0; ind < NUM[shape]; ind++)
                board2[player][shape][ind] = -1;
        }
    }

    // populate from main board
    for (int ind = 0; ind < AREA; ind++)
    {
        int tile = board[ind];
        if (tile)
        {
            bool player = player_of(tile);
            int shape = shape_of(tile);
            int *begin = board2[player][shape];
            int *end = begin + NUM[shape];
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
inline int move(bool player, int shape, int indB2, int ind_i, int ind_f, int restore = 0)
/// TODO: pawn promotion
{
    int capture = board[ind_f];
    if (capture)
    {
        int *begin = board2[!player][shape_of(capture)];
        int *end = begin + MAX_NUM;
        *std::find(begin, end, ind_f) -= AREA; // move ind_f outside board
    }
    else if (restore)
    {
        int *begin = board2[!player][shape_of(restore)];
        int *end = begin + MAX_NUM;
        *std::find(begin, end, ind_i-AREA) += AREA; // move ind_f outside the board back in
    }

    board2[player][shape][indB2] = ind_f;
    board[ind_f] = board[ind_i];
    board[ind_i] = restore;
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

inline bool is_play_area(int ind)
{
    return xy_of(ind, 0) < 8 && unsigned(ind) < AREA; // if x < 0, it becomes a huge unsigned number > AREA.
}

std::vector<int> gen_moves(bool player, int shape, int ind_i)
{
    std::vector<int> moves;
    int ind = 0, tile = 0;
    
    if (shape == KING)
    {
        for (int v: K_VECTOR)
        {
            ind = ind_i + v;
            tile = board[ind];
            if (is_play_area(ind) && (!tile || can_capture(player, tile)))
                moves.push_back(ind);
        }
    }
    if (shape == KNIGHT)
    {
        for (int v: N_VECTOR)
        {
            ind = ind_i + v;
            tile = board[ind];
            if (is_play_area(ind) && (!tile || can_capture(player, tile)))
                moves.push_back(ind);
        }
    }
    if (shape == ROOK || shape == QUEEN)
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
    if (shape == BISHOP || shape == QUEEN)
    {
        for (int v: B_VECTOR)
        {
            for (ind = ind_i + v; is_play_area(ind) && !board[ind]; ind += v)
                moves.push_back(ind);
            
            if (is_play_area(ind) && can_capture(player, board[ind]))
                moves.push_back(ind);
        }
    }
    if (shape == PAWN)
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
    ind_e = ind + rel_foward(player)*SIZE;
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
            dx = abs(xy_of(ind_e, 0) - xy_of(ind, 0));
            dy = abs(xy_of(ind_e, 1) - xy_of(ind, 1));
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
            dx = abs(xy_of(ind_e, 0) - xy_of(ind, 0));
            dy = abs(xy_of(ind_e, 1) - xy_of(ind, 1));
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
            dx = abs(xy_of(ind_e, 0) - xy_of(ind, 0));
            dy = abs(xy_of(ind_e, 1) - xy_of(ind, 1));
            if ((dx == 0 || dy == 0) && is_path_clear(ind, ind_e, dx, dy))
                return 1;
        }
    }

    // check for enemy queen
    ind_e = board2[!player][QUEEN][0];
    if (ind_e >= 0)
    {
        dx = abs(xy_of(ind_e, 0) - xy_of(ind, 0));
        dy = abs(xy_of(ind_e, 1) - xy_of(ind, 1));
        if ((dx == dy || dx == 0 || dy == 0) && is_path_clear(ind, ind_e, dx, dy))
            return 1;
    }

    // check for enemy king
    ind_e = board2[!player][KING][0];
    dx = abs(xy_of(ind_e, 0) - xy_of(ind, 0));
    dy = abs(xy_of(ind_e, 1) - xy_of(ind, 1));
    if (std::max(dx, dy) == 1)
        return 1;

    return 0;
}

void out_board()
{
    for (bool player : {BLACK, WHITE})
    {
        for (int shape = 1; shape < MAX_SHAPE; shape++)
        {
            std::cout << '[' << char_of[player][shape] << "]=";
            for (int indB2 = 0; indB2 < NUM[shape]; indB2++)
            {
                int ind = board2[player][shape][indB2];
                if (ind >= 0) // if not captured
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
            int tile = board[ind];
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

            int tile = board[ind];
            std::cout << std::setw(3) << char_of[player_of(tile)][shape_of(tile)];
        }
    }
    std::cout << "   +-------------YOU-------------+" << std::endl;
}

/**
 * Minimax + Depth-adjusted Score + AlphaBeta Prunning + Transposition Table
 * In first call, use depth = 1, alpha = INT_MIN, beta = INT_MAX.
 * @return: -depth if stalemate. INT_MAX-depth if checked by MAXER. INT_MIN-depth if checked by MINER.
 */
int minimax(bool player, int depth, int alpha, int beta)

{
    if (depth > MAX_DEPTH)
        return 0;
    
    int child_score = 0;
    for (int shape = 1; shape < MAX_SHAPE; shape++)
    {
        for (int indB2 = 0; indB2 < NUM[shape]; indB2++)
        {
            int ind_i = board2[player][shape][indB2];
            if (ind_i >= 0) // if not captured
            {
                for (int ind_f : gen_moves(player, shape, ind_i))
                {
                    int capture = move(player, shape, indB2, ind_i, ind_f);

                    if (!is_attacked(player, board2[player][KING][0]))
                    {
                        child_score = minimax(!player, depth+1, alpha, beta);

                        if (player == MAXER)
                            alpha = std::max(alpha, child_score);
                        else // if MINER
                            beta = std::min(beta, child_score);
                        if (beta <= alpha)
                        {
                            move(player, shape, indB2, ind_f, ind_i, capture); // undo move
                            break;
                        }
                    }
                    move(player, shape, indB2, ind_f, ind_i, capture); // undo move
                }
            }
        }
    }

    if (child_score == 0) // if no legal move. If move exists, minimax() never return 0 and child_score != 0.
    {
        if (is_attacked(player, board2[player][KING][0])) // checkmate
            return (player == MAXER) ? INT_MIN + depth : INT_MAX - depth;
        
        // stalemate
        return -depth;
    }
    // if has legal move
    return (player == MAXER) ? alpha : beta;
}

/**
 * Behave the same as minimax(), with the addition of recording the best move.
 */
void bot_move(bool bot)
{
    int best_indB2 = 0, best_shape = 0, best_ind_i = 0, best_ind_f = 0, best_score = 0;

    for (int shape = 1; shape < MAX_SHAPE; shape++)
    {
        for (int indB2 = 0; indB2 < NUM[shape]; indB2++)
        {
            int ind_i = board2[bot][shape][indB2];
            if (ind_i >= 0) // if not captured
            {
                std::cout << "Possible {move, score} from " << ind_i << ": ";
                for (int ind_f : gen_moves(bot, shape, ind_i))
                {
                    int capture = move(bot, shape, indB2, ind_i, ind_f);

                    if (!is_attacked(bot, board2[bot][KING][0]))
                    {
                        int score = minimax(!bot, 1, INT_MIN, INT_MAX);
                        std::cout << "{" << ind_f << ", " << score << "}, ";
                        if (bot == MAXER)
                        {
                            if (score > best_score)
                            {
                                best_indB2 = indB2;
                                best_shape = shape;
                                best_ind_i = ind_i;
                                best_ind_f = ind_f;
                                best_score = score;
                            }
                        }
                        else if (score < best_score) // if MINER
                        {
                            best_indB2 = indB2;
                            best_shape = shape;
                            best_ind_i = ind_i;
                            best_ind_f = ind_f;
                            best_score = score;
                        }
                    }
                    move(bot, shape, indB2, ind_f, ind_i, capture); // undo move
                }
                std::cout << std::endl;
            }
        }
    }
    std::cout << "Chosen move: " << best_ind_i << " to " << best_ind_f << std::endl;
    move(bot, best_shape, best_indB2, best_ind_i, best_ind_f);
}

/**
 * @return whether move is valid.
 * 0: valid, 1: broken chess rule, 2: empty initial tile, 3: outside play area, 4: friendly fire, 5: blocked path
 * TODO: 5: impersonating bot, 6: checkmate, 7: stalemate
 */
int validate(int ind_i, int ind_f)
{
    bool player = player_of(board[ind_i]);
    int shape = shape_of(board[ind_i]),
        dx = abs(xy_of(ind_f, 0) - xy_of(ind_i, 0)), dy = abs(xy_of(ind_f, 1) - xy_of(ind_i, 1));

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
        if (rel_foward(player) * (ind_f - ind_i) < 0) // moved backward
            return 1;

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
        out_board();
        do
        {
            std::cout << "Initial and final tile (ind_i ind_f): ";
            std::cin >> ind_i >> ind_f;
            invalid = validate(ind_i, ind_f);
            if (invalid)
                std::cout << "Invalid move, broken rule #" << invalid << std::endl;
        }
        while (invalid);
        // find corresponding indB2
        for (int shape = 1; shape < MAX_SHAPE; shape++)
        {
            for (int indB2 = 0; indB2 < NUM[shape]; indB2++)
            {
                if (board2[1][shape][indB2] == ind_i)
                {
                    move(1, shape, indB2, ind_i, ind_f);
                    break;
                }
            }
        }
        std::cout << std::endl;

        bot_move(0);
        std::cout << std::endl;
    }
}

int main()
{
    init_board2();
    console_play();
    return 0;
}
