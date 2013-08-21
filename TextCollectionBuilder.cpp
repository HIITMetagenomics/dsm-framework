#include "rlcsa_builder.h"
#include "TextCollectionBuilder.h"
#include "FMIndex.h"
#include "rlcsa_wrapper.h"

#include <vector>
using std::vector;
#include <string>
using std::string;

// Using pimpl idiom to hide RLCSABuilder*
struct TCBuilderRep
{
    TextCollection::IndexType type;
    unsigned samplerate;
    CSA::RLCSABuilder * sa;

    ulong n;
    // Total number of texts in the collection
    unsigned numberOfTexts;
    // Length of the longest text
    ulong maxTextLength;
    ulong numberOfSamples;
    bool insertAllowed;

    vector<string> name;
};

/**
 * Init text collection
 */
TextCollectionBuilder::TextCollectionBuilder(unsigned samplerate, ulong estimatedInputLength, TextCollection::IndexType type)
    : p_(new struct TCBuilderRep())
{
    p_->type = type;
    p_->n = 0;
    p_->samplerate = samplerate;
    if (samplerate == 0)
        p_->samplerate = TEXTCOLLECTION_DEFAULT_SAMPLERATE;

    p_->numberOfTexts = 0;
    p_->numberOfSamples = 0;
    p_->insertAllowed = true;

    CSA::usint rlcsa_block_size = CSA::RLCSA_BLOCK_SIZE.second;
    CSA::usint rlcsa_sample_rate = 0;
    if(type == TextCollection::TYPE_RLCSA)
      rlcsa_sample_rate = p_->samplerate;
 
    // Parameters for FM-index: 8 bytes, no samples, buffer size n/10 bytes.
    // Parameters for RLCSA: 32 bytes, samples, buffer size n/10 bytes.
    // Buffer size is always at least 15MB:
    if (estimatedInputLength < TEXTCOLLECTION_DEFAULT_INPUT_LENGTH)
        estimatedInputLength = TEXTCOLLECTION_DEFAULT_INPUT_LENGTH;
    p_->sa = new CSA::RLCSABuilder(rlcsa_block_size, rlcsa_sample_rate, estimatedInputLength/10);
    assert(p_->sa->isOk());
}

TextCollectionBuilder::~TextCollectionBuilder()
{
    delete p_->sa;
    delete p_;
}

void TextCollectionBuilder::InsertText(uchar const * text)
{
    if (!p_->insertAllowed)
    {
        std::cerr << "TextCollectionBuilder::InsertText() error: new text can not be inserted after InitTextCollection() call!" << std::endl;
        exit(1);
    }

    TextCollection::TextPosition m = std::strlen((char *)text) + 1;
    if (m > p_->maxTextLength)
        p_->maxTextLength = m; // Store length of the longest text seen so far.

    if (m > 1)
    {
        p_->n += m;
        p_->numberOfTexts ++;
        p_->numberOfSamples += (m-1)/p_->samplerate;

        p_->sa->insertSequence((char*)text, m-1, 0);
        assert(p_->sa->isOk());
    }
    else
    {
        // FIXME indexing empty texts
        std::cerr << "TextCollectionBuilder::InsertText() error: can not index empty texts!" << std::endl;
        exit(1);
    }
}

void TextCollectionBuilder::InsertText(uchar const * text, string const &name)
{
    p_->name.push_back(name);
    InsertText(text);
}

TextCollection * TextCollectionBuilder::InitTextCollection(bool storePlainText, bool color, unsigned rotationLength)
{
    p_->insertAllowed = false; // Disable future insertions

    TextCollection *result = 0;
    switch(p_->type) 
    {
    case(TextCollection::TYPE_FMINDEX):
    {
        uchar * bwt = 0;
        CSA::usint length = 0;
        if (p_->numberOfTexts == 0)
        {
            p_->numberOfTexts ++; // Add one empty text
            bwt = new uchar[2];
            bwt[0] = '\0';
            bwt[1] = '\0';
            length = 1;
            p_->maxTextLength = 1;
        }
        else
        {
            bwt = (uchar *)p_->sa->getBWT(length);
            delete p_->sa;
            p_->sa = 0;

            assert(length == p_->n);
        }

        result = new FMIndex(bwt, (ulong)length, p_->samplerate, p_->numberOfTexts, p_->maxTextLength, 
                             p_->numberOfSamples, p_->name, storePlainText, color, rotationLength);
        break;
    }
    case(TextCollection::TYPE_RLCSA):
        //result = new RLCSAWrapper(p_->sa->getRLCSA());
        //delete p_->sa;
        //p_->sa = 0;
        std::cerr << "TextCollectionBuilder::InitTextCollection(): currently unsupported!" << std::endl;
        std::abort();

        break;
    default:
        std::cerr << "TextCollectionBuilder::InitTextCollection(): invalid index type!" << std::endl;
        exit(2);
        break;
    }

    return result;
}
