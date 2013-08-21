/******************************************************************************
 *   Copyright (C) 2006-2010 by Veli Mäkinen and Niko Välimäki                *
 *                                                                            *
 *                                                                            *
 *   This program is free software; you can redistribute it and/or modify     *
 *   it under the terms of the GNU Lesser General Public License as published *
 *   by the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                      *
 *                                                                            *
 *   This program is distributed in the hope that it will be useful,          *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Lesser General Public License for more details.                      *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public License *
 *   along with this program; if not, write to the                            *
 *   Free Software Foundation, Inc.,                                          *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.            *
 *****************************************************************************/

#ifndef _FMIndex_H_
#define _FMIndex_H_

#include "TextCollection.h"
#include "BlockArray.h"
#include "ArrayDoc.h"
#include "TextStorage.h"

// Include  from XMLTree/libcds
#include <basics.h> // Defines W == 32
#include <static_bitsequence.h>
#include <alphabet_mapper.h>
#include <static_sequence.h>
#include <static_sequence_wvtree_noptrs.h>

// Re-define word size to ulong:
#undef W
#define W (CHAR_BIT*sizeof(unsigned long))
#undef bitset
#undef bitget

#include <string>
#include <vector>
#include <set>

/**
 * Implementation of the TextCollection interface
 *
 * Use TextCollectionBuilder to construct.
 * 
 * Thread-safe unless a Huffman shaped WT is used (see libcds).
 *
 * TODO
 *   [ ] succinct TextStoragePlainText
 */
class FMIndex : public TextCollection {
public:
    // Return name/title of given sequence
    inline std::string getName(DocId i) const
    { return std::string((char *)name->GetText(i)); }

    inline uchar const * getText(position_result const &pos)
    { return textStorage->GetText(pos.first) + pos.second; }

    // Return total lenght of text (including 0-terminators).
    inline TextPosition getLength() const
    { return n; }

    /**
     * Return lenght of i'th text (excluding 0-terminators).
     *
     * FIXME: Using too much space if the number of sequences is high.
     */
    inline TextPosition getLength(DocId i) const
    { return (*textLength)[i]; }

    // Return C[c] + rank_c(L, i) for given c and i
    inline TextPosition LF(uchar c, TextPosition i) const
    {
        if (C[(int)c+1]-C[(int)c] == 0) // FIXME fix alphabet
            return C[(int)c];
        return C[(int)c] + alphabetrank->rank(c, i);
    } 

    /**
     * Given suffix i and substring length l, return T[SA[i] ... SA[i]+l].
     *
     * Caller must delete [] the buffer.
     */ 
    uchar * getSuffix(TextPosition dest, unsigned l) const;

    inline uchar getL(TextPosition dest) const
    {
        return alphabetrank->access(dest);
    }

    // For given suffix i, return corresponding DocId and text position.
    inline void getPosition(position_result &result, TextPosition i) const
    {
        uint tmp_rank_c = 0; // Cache rank value of c.
        TextPosition dist = 0;
        uchar c = alphabetrank->access(i, tmp_rank_c);
        while (c != '\0' && !sampled->access(i))
        {
            i = C[c]+tmp_rank_c-1; //alphabetrank->rank(c,i)-1;
            c = alphabetrank->access(i, tmp_rank_c);
            ++ dist;
        }
        if (c == '\0')
        {
            // Rank among the end-markers in BWT
            unsigned endmarkerRank = tmp_rank_c-1; //alphabetrank->rank(0, i) - 1;
            result.first = Doc->access(endmarkerRank);
            result.second =  dist; 
        }
        else
        {
            result.first = (*suffixDocId)[sampled->rank1(i)-1];
            result.second = (*suffixes)[sampled->rank1(i)-1] + dist;
        }
    }

    /**
     * Enumerate document+position pairs (full_result) of
     * each suffix in given interval.
     */
    inline void getOccurrences(position_vector &result, TextPosition sp, TextPosition ep) const
    {
        result.reserve(result.size() + ep-sp+1);

        uint tmp_rank_c = 0; // Cache rank value of c.
        for (; sp <= ep; ++sp)
        {
            TextPosition i = sp;
            TextPosition dist = 0;
            uchar c = alphabetrank->access(i, tmp_rank_c);
            while (c != '\0' && !sampled->access(i))
            {
                i = C[c]+tmp_rank_c-1; //alphabetrank->rank(c,i)-1;
                c = alphabetrank->access(i, tmp_rank_c);
                ++ dist;
            }
            TextPosition textPos = (*suffixes)[sampled->rank1(i)-1] + dist;
            result.push_back(textPos);
        }
    }


    /**
     * Extracting one text.
     *
     * This call may (or may not) allocate memory depending on implementation.
     * The *current* implementation allocates memory.
     * Call DeleteText() for each pointer returned by GetText()
     * to avoid possible memory leaks.
     *
     * FIXME: Not needed?
     */
    /*uchar * GetText(DocId) const;
    inline void DeleteText(uchar *text) const
    { delete [] text; }*/

    FMIndex(uchar *, ulong, unsigned, unsigned, ulong, ulong, std::vector<std::string> &,
            bool, bool, unsigned = 0);
    // Index from/to disk
    FMIndex(std::string const &, std::string const &);
    void save(std::string const &) const;
    void saveSamples(std::string const &);
    ~FMIndex();

private:
    // Required by getSuffix(), assuming DNA alphabet
    static const char ALPHABET_SHIFTED[];

    static const uchar versionFlag;
    TextPosition n;
    unsigned samplerate;
    unsigned C[256];
    TextPosition bwtEndPos;
    static_sequence * alphabetrank;

    // Sample structures for texts longer than samplerate
    static_bitsequence * sampled;
    BlockArray *suffixes;
    BlockArray *suffixDocId;

    // Total number of texts in the collection
    unsigned numberOfTexts;
    // Length of the longest text
    ulong maxTextLength;

    // Array of document id's in the order of end-markers in BWT
    ArrayDoc *Doc;

    // Text storage for fast extraction
    TextStorage * textStorage;
    // Store text names/titles
    TextStorage * name;

    // Array of text lengths, FIXME using too much mem?
    BlockArray *textLength;

    // Following methods are not part of the public API
    uchar * BWT(uchar *);
    void makewavelet(uchar *);
    void maketables(ulong, bool);
    void makesamples();
    ulong Search(uchar const *, TextPosition, TextPosition *, TextPosition *) const;

    bool IsEndmarker(BlockArray*, TextPosition, unsigned &) const;
    TextCollection::DocId DocIdAtTextPos(BlockArray*, TextPosition) const;

    /**
     * Count end-markers in given interval
     */
    inline unsigned CountEndmarkers(TextPosition sp, TextPosition ep) const
    {
        if (sp > ep)
            return 0;

        ulong ranksp = 0;
        if (sp != 0)
            ranksp = alphabetrank->rank(0, sp - 1);
    
        return alphabetrank->rank(0, ep) - ranksp;
    }
}; // class FMIndex

#endif
