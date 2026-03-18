#include <fstream>
#ifdef _MSC_VER
    #ifndef __attribute__
        #define __attribute__(x)
    #endif
#endif
#define PCG_LITTLE_ENDIAN 1
#include "include/pcg_random.hpp"

pcg64 rng(0);

enum SHAPE: short {
    PAWN = 0, KNIGHT = 1, BISHOP = 2, ROOK = 3, QUEEN = 4, KING = 5,
    MAX_SHAPE = 6
};
enum PLAYER: short {
    BLACK = 0, WHITE = 1,
    MAX_PLAYER = 2,
    MINER = BLACK, MAXER = WHITE,
    BOT = BLACK, HUMAN = WHITE,
};

const short HEIGHT = 8, WIDTH = HEIGHT + 2, AREA = HEIGHT*WIDTH;

inline short x_of(short sq)
{
    return sq % WIDTH;
}

inline bool is_play_area(short sq)
{
    return unsigned(sq) < AREA && x_of(sq) < 8; // if x < 0, it becomes a huge unsigned number > AREA.
}

void write_fout(std::ofstream &fout)
{
    fout << "unsigned long long ZTABLE[MAX_PLAYER][MAX_SHAPE][AREA] = {";
    for (short player = BLACK; player <= WHITE; player++)
    {
        // Ztable
        fout << "{";
        for (short shape = PAWN; shape < MAX_SHAPE; shape++)
        {
            fout << "{";
            for (short sq = 0; sq < AREA; sq++)
            {
                if (is_play_area(sq))
                    fout << rng() << "ULL";
                else
                    fout << "0";
                fout << ",";
            }
            fout << "},";
        }
        fout << "},";
    }
    fout << "};\nunsigned long long Z_IS_BLACK = " << rng() << "ULL;";
}

int main()
{
    std::ofstream fout("precalc_Ztable.txt");
    write_fout(fout);
    fout.close();
}