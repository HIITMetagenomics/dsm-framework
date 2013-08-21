#ifndef _TextStorage_H_
#define _TextStorage_H_

#include "TextCollection.h"
#include "Tools.h"
#include <cassert>
#include <stdexcept>


// Include  from libcds
#include <static_bitsequence.h>

// Re-define word size to ulong:
#undef W
#define W (CHAR_BIT*sizeof(unsigned long))
#undef bitset
#undef bitget

class TextStoragePlainText;

/**
 * Text collection that supports fast extraction.
 * Defines an abstact interface class.
 * See subclasses TextStorageLzIndex and TextStoragePlainText
 * below.
 * 
 * TODO store in DNA alphabet, use delta encoded bitvector
 */
class TextStorage
{
public:
    // Define a shortcut
    typedef TextCollection::TextPosition TextPosition;
    // Storage type
    const static char TYPE_PLAIN_TEXT = 0;
    const static char TYPE_LZ_INDEX = 1;

    // Call DeleteText() for each pointer returned by GetText()
    // to avoid possible memory leaks.
    virtual uchar * GetText(TextCollection::DocId docId) const = 0;
    virtual uchar * GetText(TextCollection::DocId i, TextCollection::DocId j) const = 0;
    virtual void DeleteText(uchar *) const = 0;

    static TextStorage * Load(FILE *file);
    virtual void Save(FILE *file) const = 0;

    virtual ~TextStorage()
    {
        delete offsets_;
        offsets_ = 0;
    }

    TextCollection::DocId DocIdAtTextPos(TextCollection::TextPosition i) const
    {
        assert(i < n_);
        return offsets_->rank1(i)-1;
    }

    TextCollection::TextPosition TextStartPos(TextCollection::DocId i) const
    {
        assert(i < (TextCollection::DocId)numberOfTexts_);
        return offsets_->select1(i+1);
    }

    bool IsEndmarker(TextCollection::TextPosition i) const
    {
        assert(i < n_);
        if (i >= n_ - 1)
            return true;
        return offsets_->access(i+1);
    }

protected:
    TextStorage(uchar const * text, TextPosition n)
        : n_(n), offsets_(0), numberOfTexts_(0)
    { 
        uint *startpos = new uint[n/(sizeof(uint)*8)+1];
        for (unsigned long i = 0; i < n / (sizeof(uint)*8) + 1; i++)
            startpos[i] = 0;

        // Read offsets by finding text end positions:
        set_field(startpos,1,0,1);
        for (TextPosition i = 0; i < n_ - 1; ++i)
            if (text[i] == '\0')
                set_field(startpos,1,i+1,1);
        

        offsets_ = new static_bitsequence_brw32(startpos, n, 16);
        delete [] startpos;

        for (ulong i = 0; i < n_-1; ++i)
            if ((text[i] == '\0') != IsEndmarker(i))
                std::cout << "misplaced endmarker at i = " << i << std::endl;

        numberOfTexts_ = offsets_->rank1(n_ - 1);
    }

    TextStorage(std::FILE *);
    void Save(FILE *file, char type) const;
    TextPosition n_;

    //CSA::DeltaVector *offsets_;
    static_bitsequence * offsets_ ;
    TextPosition numberOfTexts_;
};

/******************************************************************
 * Plain text collection.
 */
class TextStoragePlainText : public TextStorage
{
public:
    TextStoragePlainText(uchar *text, TextPosition n)
        : TextStorage(text, n), text_(text)
    { }

    TextStoragePlainText(FILE *file)
        : TextStorage(file), text_(0)
    {
        text_ = new uchar[n_];
        if (std::fread(this->text_, sizeof(uchar), n_, file) != n_)
            throw std::runtime_error("TextStorage::Load(): file read error (text_).");
    }

    void Save(FILE *file) const
    {
        TextStorage::Save(file, TYPE_PLAIN_TEXT);

        if (std::fwrite(this->text_, sizeof(uchar), n_, file) != n_)
            throw std::runtime_error("TextStorage::Save(): file write error (text_).");
    }

    ~TextStoragePlainText()
    {
        delete [] text_;
        text_ = 0;
        n_ = 0;
    }

    uchar * GetText(TextCollection::DocId docId) const
    {
        assert(docId < (TextCollection::DocId)numberOfTexts_);

        TextPosition offset = offsets_->select1(docId+1);
        return &text_[offset];
    }

    uchar * GetText(TextCollection::DocId i, TextCollection::DocId j) const
    {
        assert(i < (TextCollection::DocId)numberOfTexts_);
        assert(j < (TextCollection::DocId)numberOfTexts_);

        TextPosition offset = offsets_->select1(i+1);
        return &text_[offset];
    }

    // No operation, since text is a pointer to this->text_ 
    void DeleteText(uchar *text) const
    { }

private:
    uchar *text_;
}; // class TextStoragePlainText

#endif // _TextStorage_H_
