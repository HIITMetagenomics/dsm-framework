#ifndef _BITRANK_H_
#define _BITRANK_H_
#include "Tools.h"

#include <cstdio>
#include <stdexcept>

class BitRank {
private:
#if __WORDSIZE == 32
    static const unsigned wordShift = 5;
    static const unsigned superFactor = 8; // 256 bit blocks
#else
    static const unsigned wordShift = 6;
    static const unsigned superFactor = 4; // 256 bit blocks
#endif
    
    ulong *data; //here is the bit-array
    bool owner;
    ulong n,integers;
    unsigned b,s; 
    ulong *Rs; //superblock array
    uchar *Rb; //block array

    ulong BuildRankSub(ulong,  ulong); //internal use of BuildRank
    void BuildRank(); //crea indice para rank
public:
    BitRank(ulong *, ulong, bool);
    BitRank(std::FILE *);
    ~BitRank();    
    void save(std::FILE *);

    ulong rank(ulong i) const; //Rank from 0 to n-1
    ulong rank(bool b, ulong i) const 
    {
        return b?rank(i):(i+1-rank(i));
    }
    ulong rank0(ulong i) const
    {
        return i+1-rank(i);
    }

    ulong select(ulong x) const; // gives the position of the x:th 1.
    ulong select0(ulong x) const; // gives the position of the x:th 0.

    bool IsBitSet(ulong i) const;
};

#endif
