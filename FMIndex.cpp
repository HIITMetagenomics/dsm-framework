/******************************************************************************
 *   Copyright (C) 2006-2008 by Veli Mäkinen and Niko Välimäki                *
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
#include "FMIndex.h"

//#define DEBUG_MEMUSAGE
#ifdef DEBUG_MEMUSAGE
#include "HeapProfiler.h" // FIXME remove
#endif

#include <iostream>
#include <map>
#include <utility>
#include <stdexcept>
#include <cassert>
#include <cstring> // For strlen()
using std::vector;
using std::pair;
using std::make_pair;
using std::map;

const char FMIndex::ALPHABET_SHIFTED[] = {1, ' '+1, '#'+1, 
           '.'+1, '0'+1, '1'+1, '2'+1, '3'+1,
           'A'+1, 'C'+1, 'G'+1, 'M'+1, 'N'+1, 'R'+1, 'T'+1};

/**
 * Save file version info
 *
 * v13 was using libcds! 
 * v14 uses HuffWT, 
 * v15 uses ulong C[c], 
 * v16 uses ulong codetable
 * v17 uses BitRank for sampled 
 */
const uchar FMIndex::versionFlag = 17;

/**
 * Given suffix i and substring length l, return T[SA[i] ... SA[i]+l].
 *
 * Caller must delete [] the buffer.
 */ 
uchar * FMIndex::getSuffix(TextPosition dest, unsigned l) const
{
    ++dest;
    uchar *text = new uchar[l+1];
    text[l] = 0;
    for (unsigned i = 0; i < l; ++i)
    {
        unsigned int which = 0;
        int c = 0;
        // ALPHABET_SHIFTED contains chars shifted +1
        for (const char *ch = ALPHABET_SHIFTED; ch < ALPHABET_SHIFTED+sizeof(ALPHABET_SHIFTED); ++ch) {
            c = (char)*ch;
            if (C[c] >= dest) {
                which = dest - C[c-1];
                c--;
                if (c == 0) 
                {
                    text[i] = 0;
                    return text;
                }
                break;
            }
        }
        text[i] = (uchar)c;
        dest = alphabetrank->select(c, which) + 1;
    }
    return text;
}


/**
 * Constructor inits an empty dynamic FM-index.
 * Samplerate defaults to TEXTCOLLECTION_DEFAULT_SAMPLERATE.
 */
FMIndex::FMIndex(uchar * bwt, ulong length, unsigned samplerate_, unsigned numberOfTexts_, 
                 ulong maxTextLength_, ulong numberOfSamples_, vector<std::string> &name_,
                 bool storePlainText, bool colorCoded_, unsigned rotationLength_)
    : TextCollection(colorCoded_, rotationLength_), n(length), samplerate(samplerate_), bwtEndPos(0), alphabetrank(0), 
      sampled(0), suffixes(0), suffixDocId(0), numberOfTexts(numberOfTexts_), maxTextLength(maxTextLength_), Doc(0), 
      textStorage(0), name(0), textLength(0)
{
    assert(!(rotationLength && storePlainText));
/*    if (name_.size())   FIXME disabled
    {
        // Construct name/title storage
        unsigned l = 0;
        for (vector<std::string>::const_iterator it = name_.begin(); it != name_.end(); ++it)
            l += (*it).size() + 1;
        uchar *tmp = new uchar[l];
        l = 0;
        for (vector<std::string>::const_iterator it = name_.begin(); it != name_.end(); ++it)
        {
            for (std::string::const_iterator t = (*it).begin(); t != (*it).end(); ++t)
                tmp[l++] = *t;
            tmp[l++] = 0;
        }
        name_.clear(); // discard the vector
        this->name = new TextStoragePlainText(tmp, l);
        }*/

    makewavelet(bwt); // Deletes bwt!
    bwt = 0;
 
    // Make sampling tables (FIXME disabled)
    //maketables(numberOfSamples_, storePlainText);
}

void FMIndex::saveSamples(std::string const & filename)
{
//    makesamples();
    cerr << "Saving samples for samplerate = " << samplerate << "..." << endl;
    maketables(n/samplerate+1, false);

    std::string name = filename + ".sa";
    std::FILE *file = std::fopen(name.c_str(), "wb");

    if (sampled)
        sampled->save(file);
    if (suffixes)
        suffixes->Save(file);
    if (suffixDocId)
        suffixDocId->Save(file);
    if (textLength)
        textLength->Save(file);
    if (Doc)
        Doc->save(file);

    fflush(file);
    std::fclose(file);
}

/**
 * Save index to a file handle
 *
 * Throws a std::runtime_error exception on i/o error.
 * First byte that is saved represents the version number of the save file.
 */
void FMIndex::save(std::string const & filename) const
{
    std::string name = filename + TextCollection::FMINDEX_EXTENSION;
    std::FILE *file = std::fopen(name.c_str(), "wb");


    // Saving version info:
    if (std::fwrite(&versionFlag, 1, 1, file) != 1)
        throw std::runtime_error("FMIndex::save(): file write error (version flag).");

    if (std::fwrite(&(this->n), sizeof(TextPosition), 1, file) != 1)
        throw std::runtime_error("FMIndex::save(): file write error (n).");
    if (std::fwrite(&(this->samplerate), sizeof(unsigned), 1, file) != 1)
        throw std::runtime_error("FMIndex::save(): file write error (samplerate).");

    if (std::fwrite(this->C, sizeof(ulong), 256, file) != 256)
        throw std::runtime_error("FMIndex::save(): file write error (C table).");

    if (std::fwrite(&(this->bwtEndPos), sizeof(TextPosition), 1, file) != 1)
        throw std::runtime_error("FMIndex::save(): file write error (bwt end position).");
        
//    alphabetrank->save(file);
    HuffWT::save(alphabetrank, file);

    if (suffixDocId)
        suffixDocId->Save(file);
    if (textLength)
        textLength->Save(file);
    
    if (std::fwrite(&(this->numberOfTexts), sizeof(unsigned), 1, file) != 1)
        throw std::runtime_error("FMIndex::save(): file write error (numberOfTexts).");
    if (std::fwrite(&(this->maxTextLength), sizeof(ulong), 1, file) != 1)
        throw std::runtime_error("FMIndex::save(): file write error (maxTextLength).");

    if (Doc)
        Doc->save(file);

    // TODO clean up
    bool flag = false;
    if (this->name)
        flag = true;
    if (std::fwrite(&flag, sizeof(bool), 1, file) != 1)
        throw std::runtime_error("FMIndex::save(): file write error (name flag).");
    if (this->name)
        this->name->Save(file);

    flag = false;
    if (textStorage)
        flag = true;
    if (std::fwrite(&flag, sizeof(bool), 1, file) != 1)
        throw std::runtime_error("FMIndex::save(): file write error (text storage flag).");
    if (textStorage)
        textStorage->Save(file);

    // TODO these are from base class
    if (std::fwrite(&colorCoded, sizeof(bool), 1, file) != 1)
        throw std::runtime_error("FMIndex::save(): file write error (color flag).");
    if (std::fwrite(&rotationLength, sizeof(unsigned), 1, file) != 1)
        throw std::runtime_error("FMIndex::save(): file write error (rotation length).");

    fflush(file);
    std::fclose(file);
}

void FMIndex::recomputeC()
{
    ulong i;
    for (i=0;i<256;i++)
        C[i]=0;
    for (i=0;i<n;++i)
    {
        if (i%100000000 == 0)
            cerr << "recomputing C, i = " << i << endl;
        C[(int)alphabetrank->access(i)]++;
    }
    ulong prev=C[0], temp;
    C[0]=0;
    for (i=1;i<256;i++) {          
        temp = C[i];
        C[i]=C[i-1]+prev;
        prev = temp;
    }
}

/**
 * Load index from a file handle
 *
 * Throws a std::runtime_error exception on i/o error.
 * For more info, see FMIndex::save().
 */
FMIndex::FMIndex(std::string const & filename, std::string const & samplefile = "")
    : n(0), samplerate(0), alphabetrank(0), sampled(0), suffixes(0), 
      suffixDocId(0), numberOfTexts(0), maxTextLength(0), Doc(0), 
      textStorage(0), name(0), textLength(0)
{
    std::string name = filename + TextCollection::FMINDEX_EXTENSION;
    std::FILE *file = std::fopen(name.c_str(), "rb");
    if (file == NULL)
        throw std::runtime_error("file not found");

    std::FILE *safile = 0;
    if (samplefile != "")
    {
        name = samplefile + ".sa";
        safile = std::fopen(name.c_str(), "rb");
        if (safile == NULL)
            throw std::runtime_error("sample file not found");
    }

    uchar verFlag = 0;
    if (std::fread(&verFlag, 1, 1, file) != 1)
        throw std::runtime_error("file read error: incorrect version flag! Please reconstruct the index");
    if (verFlag != FMIndex::versionFlag && verFlag != 15 && verFlag != 14 && verFlag != 16)
        throw std::runtime_error("FMIndex::FMIndex(): invalid save file version.");
//    cerr << "verFlag = " << (int)verFlag << endl;
    if (std::fread(&(this->n), sizeof(TextPosition), 1, file) != 1)
        throw std::runtime_error("FMIndex::FMIndex(): file read error (n).");
    if (std::fread(&samplerate, sizeof(unsigned), 1, file) != 1)
        throw std::runtime_error("FMIndex::FMIndex(): file read error (samplerate).");
// FIXME samplerate can not be changed during load.
//    if (this->samplerate == 0)
//        this->samplerate = samplerate;

    if (verFlag == 14)
    {
        // Values are stored in unsigned size variable
        for(ulong i = 0; i < 256; ++i)
        {
            unsigned j = 0;
            if (std::fread(&j, sizeof(unsigned), 1, file) != 1)
            throw std::runtime_error("FMIndex::FMIndex(): file read error (C table).");
            C[i] = j;
        }
    }
    else
        if (std::fread(this->C, sizeof(ulong), 256, file) != 256)
            throw std::runtime_error("FMIndex::FMIndex(): file read error (C table).");

    if (std::fread(&(this->bwtEndPos), sizeof(TextPosition), 1, file) != 1)
        throw std::runtime_error("FMIndex::FMIndex(): file read error (bwt end position).");

    //alphabetrank = static_sequence::load(file);
    alphabetrank = HuffWT::load(file, verFlag);

    if (safile)
    {
        if (verFlag == 16)
            cerr << "Warning: FMIndex::FMIndex(): invalid save file version?" << endl;
        sampled = new BitRank(safile); //static_bitsequence::load(safile);
        suffixes = new BlockArray(safile);
        suffixDocId = new BlockArray(safile);
        textLength = new BlockArray(safile);
        Doc = new ArrayDoc(safile);
    }
/*    suffixDocId = new BlockArray(file);  FIXME disabled
    textLength = new BlockArray(file);*/

    if (std::fread(&(this->numberOfTexts), sizeof(unsigned), 1, file) != 1)
        throw std::runtime_error("FMIndex::FMIndex(): file read error (numberOfTexts).");
    if (std::fread(&(this->maxTextLength), sizeof(ulong), 1, file) != 1)
        throw std::runtime_error("FMIndex::FMIndex(): file read error (maxTextLength).");

//    Doc = new ArrayDoc(file);  FIXME disabled

    // Read text names/titles
    // If first byte == 0, ignore
    // TODO clean up
    bool flag = false;
    if (std::fread(&flag, sizeof(bool), 1, file) != 1)
        throw std::runtime_error("FMIndex::FMIndex(): file read error (name flag).");
    if (flag)
        this->name = TextStorage::Load(file);

    flag = false;
    if (std::fread(&flag, sizeof(bool), 1, file) != 1)
        throw std::runtime_error("FMIndex::FMIndex(): file read error (name flag).");
    if (flag)
        this->textStorage = TextStorage::Load(file);

    if (std::fread(&colorCoded, sizeof(bool), 1, file) != 1)
        throw std::runtime_error("FMIndex::FMIndex(): file read error (color flag).");
    if (std::fread(&rotationLength, sizeof(unsigned), 1, file) != 1)
        throw std::runtime_error("FMIndex::FMIndex(): file read error (rotation length).");
    
    assert(!(this->textStorage && rotationLength));

    std::fclose(file);
    // FIXME Construct data structures with new samplerate
    //maketables(); 


    for(ulong i = 1; i < 256; ++i)
    {
//        cerr << "C[" << i << "] = " << C[i] << endl;
        if (i && C[i] < C[i-1])
        {
            cerr << "C has truncated values, recomputing... version = " << (int)verFlag << endl;
            recomputeC();
            save(name + ".reC");
            break;
        }
    }
}


ulong FMIndex::Search(uchar const * pattern, TextPosition m, TextPosition *spResult, TextPosition *epResult) const 
{
    // use the FM-search replacing function Occ(c,1,i) with alphabetrank->rank(c,i)
    int c = (int)pattern[m-1]; 
    //fprintf(stderr,"c=%d\n",c);
    TextPosition i=m-1;
    TextPosition sp = C[c];
    TextPosition ep = C[c+1]-1;
    while (sp<=ep && i>=1) 
    {
//         printf("i = %lu, c = %c, sp = %lu, ep = %lu\n", i, pattern[i], sp, ep);
        c = (int)pattern[--i];
        sp = C[c]+alphabetrank->rank(c,sp-1);
        ep = C[c]+alphabetrank->rank(c,ep)-1;
    }
    *spResult = sp;
    *epResult = ep;
    if (sp<=ep)
        return ep - sp + 1;
    else
        return 0;
}


FMIndex::~FMIndex() {
    HuffWT::deleteHuffWT(alphabetrank);
    delete sampled;
    delete suffixes;
    delete suffixDocId;
    delete Doc;
    delete textLength;
    delete textStorage;
    delete name;
}

void FMIndex::makewavelet(uchar *bwt)
{
    ulong i;
    for (i=0;i<256;i++)
        C[i]=0;
    for (i=0;i<n;++i)
        C[(int)bwt[i]]++;
    
    ulong prev=C[0], temp;
    C[0]=0;
    for (i=1;i<256;i++) {          
        temp = C[i];
        C[i]=C[i-1]+prev;
        prev = temp;
    }

/*    alphabet_mapper * am = new alphabet_mapper_none();
    static_bitsequence_builder * bmb = new static_bitsequence_builder_brw32(8); // FIXME samplerate?
    wt_coder * wtc = new wt_coder_huff(bwt,n,am); // FIXME Huffman is not thread-safe
    alphabetrank = new static_sequence_wvtree(bwt,n,wtc,bmb,am);
    delete bmb;
    bwt = 0; // already deleted
*/


    std::cerr << "Constructing HuffWT.." << std::endl;
    alphabetrank = HuffWT::makeHuffWT(bwt, n);
    std::cerr << "Done." << std::endl;
//    delete [] bwt; // Was deleted
    bwt = 0;
}

unsigned FMIndex::outputReads(ResultSet *results)
{
    // Calculate BWT end-marker position (of last inserted text)
    {
        ulong i = 0;
        ulong alphabetrank_i_tmp = 0;
        uchar c  = alphabetrank->access(i, alphabetrank_i_tmp);
        while (c != '\0')
        {
            i = C[c]+alphabetrank_i_tmp-1;
            c = alphabetrank->access(i, alphabetrank_i_tmp);
        }

        this->bwtEndPos = i;
    }

    string curRead;
    curRead.reserve(1024);

    DocId textId = numberOfTexts;
    assert(textId != 0);
    ulong p=bwtEndPos;
    unsigned nreads = 0;
    ulong ulongmax = 0; ulongmax--;
    ulong alphabetrank_i_tmp =0;
    bool output = false;
    for (ulong i=n-1;i<ulongmax;i--) 
    {        
        if (i % 100000000 == 0)
            cerr << "i = " << i << ", processing..." << endl;
        uchar c = alphabetrank->access(p, alphabetrank_i_tmp);
        if (results->get(p))
            output = true;

        if (c == '\0')
        {
            --textId;

            if (output)
            {
                ++nreads;
                printf("> %u\n%s\n", textId, curRead.c_str());
            }
            curRead.clear();
            output = false;

            // LF-mapping from '\0' does not work with this (pseudo) BWT (see details from Wolfgang's thesis).
            p = textId; // Correct LF-mapping to the last char of the previous text.
        }
        else // Now c != '\0', do LF-mapping:
        {
            curRead.push_back((char)c);
            p = C[c]+alphabetrank_i_tmp-1;
        }
    }
    assert(textId == 0);
    return nreads;
}

void FMIndex::makesamples()
{
    // Calculate BWT end-marker position (of last inserted text)
    {
        ulong i = 0;
        ulong alphabetrank_i_tmp = 0; // ...was uint originally
        uchar c  = alphabetrank->access(i, alphabetrank_i_tmp);
        while (c != '\0')
        {
            i = C[c]+alphabetrank_i_tmp-1;
            c = alphabetrank->access(i, alphabetrank_i_tmp);
        }

        this->bwtEndPos = i;
    }

    BlockArray* positions = new BlockArray(n/samplerate+numberOfTexts, Tools::CeilLog2(this->n));
    ulong *sampledpositions = new ulong[n/(sizeof(ulong)*8)+1];
    for (ulong i = 0; i < n / (sizeof(ulong)*8) + 1; i++)
        sampledpositions[i] = 0;
    
    DocId textId = numberOfTexts;
    ulong p=bwtEndPos;
    ulong sampleCount = 0;
    ulong ulongmax = 0; ulongmax--;
    ulong alphabetrank_i_tmp =0;
    for (ulong i=n-1;i<ulongmax;i--) 
    {        
        uchar c = alphabetrank->access(p, alphabetrank_i_tmp);
        if (i % samplerate == 0 || c == '\0')
        {
            Tools::SetField(sampledpositions,1,p,1);
            (*positions)[sampleCount] = p;
            sampleCount ++;
        }

        if (c == '\0')
        {
            --textId;
            // LF-mapping from '\0' does not work with this (pseudo) BWT (see details from Wolfgang's thesis).
            p = textId; // Correct LF-mapping to the last char of the previous text.
        }
        else // Now c != '\0', do LF-mapping:
            p = C[c]+alphabetrank_i_tmp-1;
    }
    assert(textId == 0);

    //sampled = new static_bitsequence_brw32(sampledpositions, n, 16);
    //delete [] sampledpositions;
    sampled = new BitRank(sampledpositions, n, true);
    cerr << "rank1 = " << sampled->rank(n-1) << ", samplec = " << sampleCount << endl;

    // Suffixes store an offset from the text start position
    suffixes = new BlockArray(sampleCount, Tools::CeilLog2(n));

    ulong x = n - 1;
    p = bwtEndPos;
    textId = numberOfTexts;
    for(ulong i=0; i<sampleCount; i++) {
        // Find next sampled text position
        uchar c = alphabetrank->access(p, alphabetrank_i_tmp);        
        while (x % samplerate != 0 && c != '\0')
        {
            --x;
            p = C[c]+alphabetrank_i_tmp-1; // LF mapping
            c = alphabetrank->access(p, alphabetrank_i_tmp);
        }
        assert((*positions)[i] < n);
        ulong j = sampled->rank((*positions)[i]);

        assert(j != 0);
        (*suffixes)[j-1] = (x==n-1) ? 0 : x+1;
        --x;
        if (c == '\0')
        {
            --textId;
            // LF-mapping from '\0' does not work with this (pseudo) BWT (see details from Wolfgang's thesis).
            p = textId; // Correct LF-mapping to the last char of the previous text.
        }
        else // Now c != '\0', do LF-mapping:
            p = C[c]+alphabetrank_i_tmp-1;
    }

    delete positions;
}

void FMIndex::maketables(ulong sampleLength, bool storePlainText)
{
    // Calculate BWT end-marker position (of last inserted text)
    {
        ulong i = 0;
        ulong alphabetrank_i_tmp = 0;
        uchar c  = alphabetrank->access(i, alphabetrank_i_tmp);
        while (c != '\0')
        {
            i = C[c]+alphabetrank_i_tmp-1;
            c = alphabetrank->access(i, alphabetrank_i_tmp);
        }

        this->bwtEndPos = i;
    }

    // Build up array for text starting positions
    BlockArray* textStartPos = new BlockArray(numberOfTexts, Tools::CeilLog2(this->n));
    (*textStartPos)[0] = 0; 

    // Mapping from end-markers to doc ID's:
    unsigned logNumberOfTexts = Tools::CeilLog2(numberOfTexts);
    BlockArray *endmarkerDocId = new BlockArray(numberOfTexts, logNumberOfTexts);

    BlockArray* positions = new BlockArray(sampleLength, Tools::CeilLog2(this->n));
    ulong *sampledpositions = new ulong[n/(sizeof(ulong)*8)+1];
    for (ulong i = 0; i < n / (sizeof(ulong)*8) + 1; i++)
        sampledpositions[i] = 0;
    
    ulong x,p=bwtEndPos;
    ulong sampleCount = 0;
    // Keeping track of text position of prev. end-marker seen
    ulong posOfSuccEndmarker = n-1;
    DocId textId = numberOfTexts;
    ulong ulongmax = 0;
    ulongmax--;
    ulong alphabetrank_i_tmp =0;

    uchar *plainText = 0;
    if (storePlainText)
        plainText = new uchar[n];
    ulong pt_i = n; // Iterator from text length to 0.
    
    for (ulong i=n-1;i<ulongmax;i--) {
        // i substitutes SA->GetPos(i)
        x=(i==n-1)?0:i+1;

        uchar c = alphabetrank->access(p, alphabetrank_i_tmp);

        if (plainText)
            plainText[--pt_i] = c; // Build TextStorage

        if ((posOfSuccEndmarker - i) % samplerate == 0 && c != '\0')
        {
            Tools::SetField(sampledpositions,1,p,1); //set_field(sampledpositions,1,p,1);
            (*positions)[sampleCount] = p;
            sampleCount ++;
        }

        if (c == '\0')
        {
            --textId;
            
            // Record the order of end-markers in BWT:
            ulong endmarkerRank = alphabetrank_i_tmp - 1;
            (*endmarkerDocId)[endmarkerRank] = (textId + 1) % numberOfTexts;
            

            // Store text length and text start position:
            if (textId < (DocId)numberOfTexts - 1)
            {
                (*textStartPos)[textId + 1] = x;  // x-1 is text position of end-marker.
                posOfSuccEndmarker = i;
            }

            // LF-mapping from '\0' does not work with this (pseudo) BWT (see details from Wolfgang's thesis).
            p = textId; // Correct LF-mapping to the last char of the previous text.
        }
        else // Now c != '\0', do LF-mapping:
            p = C[c]+alphabetrank_i_tmp-1;
    }
    assert(textId == 0);
    assert(pt_i == 0 || pt_i == n);

    if (plainText)
    {
        assert(pt_i == 0);
        textStorage = new TextStoragePlainText(plainText,n);
    }
    else
        textStorage = 0;

    //sampled = new static_bitsequence_brw32(sampledpositions, n, 16);
    //delete [] sampledpositions;
    sampled = new BitRank(sampledpositions, n, true);
    cerr << "rank1 = " << sampled->rank(n-1) << ", samplec = " << sampleCount << endl;

    // Suffixes store an offset from the text start position
    suffixes = new BlockArray(sampleCount, Tools::CeilLog2(maxTextLength));
    suffixDocId = new BlockArray(sampleCount, Tools::CeilLog2(numberOfTexts));

    x = n - 2;
    posOfSuccEndmarker = n-1;
    textId = numberOfTexts - 1;
    for(ulong i=0; i<sampleCount; i++) {
        // Find next sampled text position
        while ((posOfSuccEndmarker - x) % samplerate != 0)
        {
            --x;
            assert(x != ~0lu);
            if (IsEndmarker(textStartPos, x, textId))
                posOfSuccEndmarker = x--;
        }
        assert((*positions)[i] < n);
        ulong j = sampled->rank((*positions)[i]);

        assert(j != 0); // if (j==0) j=sampleCount;
        
        TextPosition textPos = (x==n-1)?0:x+1;
        (*suffixDocId)[j-1] = DocIdAtTextPos(textStartPos, textPos);

        assert((*suffixDocId)[j-1] < numberOfTexts);
        // calculate offset from text start:
        (*suffixes)[j-1] = textPos - (*textStartPos)[(*suffixDocId)[j-1]];
        --x;
        if (x != ~0lu && IsEndmarker(textStartPos, x, textId))
            posOfSuccEndmarker = x--;
    }

    delete positions;

    // Record text lengths
    textLength = new BlockArray(numberOfTexts, Tools::CeilLog2(maxTextLength));
    for (unsigned i = 0; i < numberOfTexts - 1; ++i)
        (*textLength)[i] = (*textStartPos)[i+1] - (*textStartPos)[i] - 1;
    (*textLength)[numberOfTexts-1] = n - (*textStartPos)[numberOfTexts-1] - 1;
    
    delete textStartPos;

    Doc = new ArrayDoc(endmarkerDocId);


}

/**
 * Check if given position is end-marker
 * 
 * Third parameter counts from numberOfTexts-1 to 0.
 */
bool FMIndex::IsEndmarker(BlockArray* textStartPos, TextPosition i, unsigned &d) const
{
    if (d == 0)
        return false;

    if ((*textStartPos)[d] - 1 == i)
    {
        --d; // Move to preceeding text start pos
        return true;
    }
    return false;
}

/**
 * Finds document identifier for given text position
 *
 * Binary searching on text starting positions. 
 * Slow but used only during construction of SA samples.
 */
TextCollection::DocId FMIndex::DocIdAtTextPos(BlockArray* textStartPos, TextPosition i) const
{
    assert(i < n);

    DocId a = 0;
    DocId b = numberOfTexts - 1;
    while (a < b)
    {
        DocId c = a + (b - a)/2;
        if ((*textStartPos)[c] > i)
            b = c - 1;
        else if ((*textStartPos)[c+1] > i)
            return c;
        else
            a = c + 1;
    }

    assert(a < (DocId)numberOfTexts);
    assert(i >= (*textStartPos)[a]);
    assert(i < (a == (DocId)numberOfTexts - 1 ? n : (*textStartPos)[a+1]));
    return a;
}

