/******************************************************************************
 *   Copyright (C) 2008 by Niko Valimaki <nvalimak@cs.helsinki.fi>            *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.                *
 ******************************************************************************/ 

#ifndef _TextCollection_h_
#define _TextCollection_h_

#include "Tools.h" // Defines ulong and uchar.
#include <string>
#include <vector>
#include <utility> // Defines std::pair.

/**
 * General interface for a text collection
 *
 * Class is virtual, make objects using the class
 * TextCollectionBuilder.
 *
 * Most of the following methods should be "const" and thread-safe.
 * Libcds is thread-safe for most (obvious) parts, except for the Huffman shaped WT.
 */
class TextCollection
{
public:

    // Type of document identifier
    typedef unsigned DocId;
    // Type for text position
    typedef ulong TextPosition;
    // Data type for results
    typedef std::pair<DocId, TextPosition> position_result;
    typedef std::vector<TextPosition> position_vector;

    enum IndexType {TYPE_FMINDEX, TYPE_RLCSA};
    static const std::string REVERSE_EXTENSION;
    static const std::string ROTATION_EXTENSION;
    static const std::string FMINDEX_EXTENSION;
    static const std::string RLCSA_EXTENSION;
    
    // Return name/title of given sequence
    virtual std::string getName(DocId) const = 0;

    //    virtual unsigned outputReads(ResultSet *) = 0; 

    // Return plain sequence
//    virtual uchar const * getText(position_result const &) = 0;

    // Return total lenght of text (including 0-terminators).
    virtual TextPosition getLength() const = 0;
    virtual DocId getNumberOfTexts() const = 0;

    // Return lenght of i'th text (excluding 0-terminators).
    virtual TextPosition getLength(DocId) const = 0;

    // Return C[c] + rank_c(L, i) for given c and i
    virtual TextPosition LF(uchar, TextPosition) const = 0; 

    /**
     * Given suffix i and substring length l, return T[SA[i] ... SA[i]+l].
     *
     * Caller must delete [] the buffer.
     */ 
    virtual uchar * getSuffix(TextPosition, unsigned) const = 0; 
    virtual uchar getL(TextPosition) const = 0; 

    // For given suffix (type TextPosition), return corresponding DocId and text position.
    virtual void getPosition(position_result &, TextPosition) const = 0;

    /**
     * Enumerate all document+position pairs corresponding to
     * suffixes in given interval.
     */
    virtual void getOccurrences(position_vector &result, TextPosition sp, TextPosition ep) const = 0;

    /**
     * Load index from a file
     *
     * Subclass is selected based on the suffix of given filename.
     * Valid suffixes are TextCollection::FMINDEX_EXTENSION 
     * and TextCollection::RLCSA_EXTENSION.
     * If suffix is not specified/valid, both filename extensions
     * are checked.
     */
    static TextCollection* load(std::string const &, std::string const & = "");

    /**
     * Save data structure into a file
     */
    virtual void save(std::string const &) const = 0;
    virtual void saveSamples(std::string const &) = 0;

    virtual ~TextCollection() { };

    // Accessors for options
    bool isColorCoded() const
    { return colorCoded; }
    unsigned getRotationLength() const
    { return rotationLength; }
    

protected:
    // use TextCollectionBuilder
    TextCollection(bool cc = false, unsigned rl = 0) 
        : colorCoded(cc), rotationLength(rl)
    { };

    // Settings
    bool colorCoded;         // true if sequences are in color bases
    unsigned rotationLength; // == 0 if not a rotation index
private:
    // No copy constructor or assignment
    TextCollection(TextCollection const&);
    TextCollection& operator = (TextCollection const&);
};

#endif
