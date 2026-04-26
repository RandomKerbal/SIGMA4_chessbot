#include <fstream>
#ifdef _MSC_VER
    #ifndef __attribute__
        #define __attribute__(x)
    #endif
#endif
#define PCG_LITTLE_ENDIAN 1
#include "../include/pcg_random.hpp"

pcg32 rng32(0);
pcg64 rng(0);

enum SHAPE: short {
    PAWN = 0, KNIGHT = 1, BISHOP = 2, ROOK = 3, QUEEN = 4, KING = 5,
    MAX_SHAPE = 6
};

enum Player: short {
    BOT = 0, HUMAN = 1,
    MAX_PLAYER = 2,
    MINER = BOT, MAXER = HUMAN
};

enum Side: bool {
    Q_SIDE = 0, K_SIDE = 1
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

void write_fout(std::ofstream &fout, bool is_32)
{
    std::string datatype, suffix;
    if (is_32)
    {
        datatype = "unsigned long ";
        suffix = "UL";
    }
    else
    {
        datatype = "unsigned long long ";
        suffix = "ULL";
    }

    fout << datatype << "ZTABLE[MAX_PLAYER][MAX_SHAPE][AREA] = {";
    for (short player = BOT; player <= HUMAN; player++)
    {
        fout << "{";
        for (short shape = PAWN; shape < MAX_SHAPE; shape++)
        {
            fout << "{";
            for (short sq = 0; sq < AREA; sq++)
            {
                if (is_play_area(sq))
                    fout << (is_32 ? rng32() : rng()) << suffix;
                else
                    fout << "0";
                fout << ",";
            }
            fout << "},";
        }
        fout << "},";
    }
    fout << "};\n";
    fout << datatype << "ZCASTLE[MAX_PLAYER][2] = {";
    for (short player = BOT; player <= HUMAN; player++)
    {
        fout << "{";
        for (short side = Q_SIDE; side <= K_SIDE; side++)
            fout << (is_32 ? rng32() : rng()) << suffix << ",";
        fout << "},";   
    }
    fout << "};\n";
    fout << datatype << "ZPLAYER = " << (is_32 ? rng32() : rng()) << suffix << ";";
}

int main()
{
    std::ofstream fout("precalc_Ztable.txt");
    std::ofstream fout32("precalc_Ztable32.txt");
    write_fout(fout, false);
    write_fout(fout32, true);
    fout.close();
    fout32.close();
}