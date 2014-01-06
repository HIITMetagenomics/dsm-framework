/******************************************************************************
 *   Copyright (C) 2009 by Niko Valimaki <nvalimak@cs.helsinki.fi>            *
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

#ifndef _SXSI_TextCollectionBuilder_h_
#define _SXSI_TextCollectionBuilder_h_

#include "TextCollection.h"
#include "Tools.h" // Defines ulong and uchar.
#include <vector>
#include <utility> // Defines std::pair.
#include <cstring> // Defines std::strlen

// Default samplerate for suffix array samples
#define TEXTCOLLECTION_DEFAULT_SAMPLERATE 124

// Default input length, used to calculate the buffer size.
#define TEXTCOLLECTION_DEFAULT_INPUT_LENGTH (5lu * 1024 * 1024 * 1024)


struct TCBuilderRep; // Pimpl
    
/**
 * Build an instance of the TextCollection class.
 */
class TextCollectionBuilder
{
public:
    explicit TextCollectionBuilder(unsigned samplerate = TEXTCOLLECTION_DEFAULT_SAMPLERATE, 
                                   ulong estimatedInputLength =  TEXTCOLLECTION_DEFAULT_INPUT_LENGTH,
                                   TextCollection::IndexType type = TextCollection::TYPE_FMINDEX);
    ~TextCollectionBuilder();
        
    /** 
     * Insert text
     *
     * Must be a zero-terminated string from alphabet [1,255].
     * Can not be called after makeStatic().
     * The i'th text insertion gets an identifier value i-1.
     * In other words, document identifiers start from 0.
     */
    void InsertText(uchar const *);
    void InsertText(uchar const *, std::string const &);

    /**
     * Init the static index
     *
     * New texts can not be inserted after this operation.
     */
    TextCollection * InitTextCollection(bool = false, bool = false, unsigned = 0);
        
private:
    struct TCBuilderRep * p_;

    // No copy constructor or assignment
    TextCollectionBuilder(TextCollectionBuilder const&);
    TextCollectionBuilder& operator = (TextCollectionBuilder const&);
};

#endif
