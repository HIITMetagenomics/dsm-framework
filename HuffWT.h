#ifndef _HUFFWT_H_
#define _HUFFWT_H_


#include "BitRank.h"

#include <cstdio>
#include <stdexcept>

class HuffWT 
{
public:
    class TCodeEntry 
    {
    public:
        ulong count;
        unsigned bits;
        unsigned code;
        TCodeEntry() {count=0;bits=0;code=0u;};
        void load(std::FILE *file, uchar verFlag)
        {
            if (verFlag < 16)
            {
                unsigned temp = 0;
                if (std::fread(&temp, sizeof(unsigned), 1, file) != 1)
                    throw std::runtime_error("TCodeEntry: file read error (Rs).");
                count = temp;
            }
            else
                if (std::fread(&count, sizeof(ulong), 1, file) != 1)
                    throw std::runtime_error("TCodeEntry: file read error (Rs).");
            if (std::fread(&bits, sizeof(unsigned), 1, file) != 1)
                throw std::runtime_error("TCodeEntry: file read error (Rs).");
            if (std::fread(&code, sizeof(unsigned), 1, file) != 1)
                throw std::runtime_error("TCodeEntry: file read error (Rs).");
        }
        void save(std::FILE *file)
        {
            if (std::fwrite(&count, sizeof(ulong), 1, file) != 1)
                throw std::runtime_error("TCodeEntry: file write error (Rs).");
            if (std::fwrite(&bits, sizeof(unsigned), 1, file) != 1)
                throw std::runtime_error("TCodeEntry: file write error (Rs).");
            if (std::fwrite(&code, sizeof(unsigned), 1, file) != 1)
                throw std::runtime_error("TCodeEntry: file write error (Rs).");
        }
    };

private:
    BitRank *bitrank;
    HuffWT *left;
    HuffWT *right;
    TCodeEntry *codetable;
    uchar ch;
    bool leaf;

    HuffWT(uchar *, ulong, TCodeEntry *, unsigned);
    void save(std::FILE *);
    HuffWT(std::FILE *, TCodeEntry *);
public:
    static HuffWT * makeHuffWT(uchar *bwt, ulong n);
    static HuffWT * load(std::FILE *, uchar verFlag);
    static void save(HuffWT *, std::FILE *);
    static void deleteHuffWT(HuffWT *);
    ~HuffWT(); const
        
    inline ulong rank(uchar c, ulong i) const { // returns the number of characters c before and including position i
        HuffWT const *temp=this;
        if (codetable[c].count == 0) return 0;
        unsigned level = 0;
        unsigned code = codetable[c].code;
        while (!temp->leaf) {
            if ((code & (1u<<level)) == 0) {
                i = i-temp->bitrank->rank(i); 
                temp = temp->left; 
            }
            else { 
                i = temp->bitrank->rank(i)-1; 
                temp = temp->right;
            }
            ++level;
        } 
        return i+1;
    };   

    inline ulong select(uchar c, ulong i, unsigned level = 0) const 
    {
        if (leaf)
            return i-1;

//        if (codetable[c].count == 0) return 0;
//        unsigned level = 0;
        unsigned code = codetable[c].code;
            if ((code & (1u<<level)) == 0) {
                i = left->select(c, i, level+1);
                i = bitrank->select0(i+1); 
            }
            else 
            {
                i = right->select(c, i, level+1);
                i = bitrank->select(i+1); 
            }
        return i;
    };   

    inline bool IsCharAtPos(uchar c, ulong i) 
    {
        HuffWT const *temp=this;
        if (codetable[c].count == 0) return false;
        unsigned level = 0;
        unsigned code = codetable[c].code;      
        while (!temp->leaf) {
            if ((code & (1u<<level))==0) {
                if (temp->bitrank->IsBitSet(i)) return false;
                i = i-temp->bitrank->rank(i); 
                    temp = temp->left; 
            }
            else { 
                if (!temp->bitrank->IsBitSet(i)) return false;         
                i = temp->bitrank->rank(i)-1; 
                temp = temp->right;
            }
            ++level;
        } 
        return true;
    }
    inline uchar access(ulong i) const 
    {
        HuffWT const *temp=this;
        while (!temp->leaf) {
            if (temp->bitrank->IsBitSet(i)) {
                i = temp->bitrank->rank(i)-1;
                temp = temp->right;
            }
            else {
                i = i-temp->bitrank->rank(i); 
                temp = temp->left;      
            }         
        }
        return (int)temp->ch;
    }

    inline uchar access(ulong i, ulong &rank) const
    {
        HuffWT const *temp=this;
        while (!temp->leaf) {
            if (temp->bitrank->IsBitSet(i)) {
                i = temp->bitrank->rank(i)-1;
                temp = temp->right;
            }
            else {
                i = i-temp->bitrank->rank(i); 
                temp = temp->left;      
            }         
        }
        rank = i+1;
        return (int)temp->ch;
    }
};
#endif
