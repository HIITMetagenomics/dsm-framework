#ifndef RLCSA_WRAPPER_H
#define RLCSA_WRAPPER_H

#include "rlcsa.h"
#include "TextCollection.h"


/**
 * A simple thread-safe wrapper for RLCSA.
 *
 * Use TextCollectionBuilder to construct.
 */
class RLCSAWrapper : public TextCollection
{
  public:
    RLCSAWrapper(const CSA::RLCSA* index);
    RLCSAWrapper(const std::string& base_name);
    ~RLCSAWrapper();

    void save(const std::string& base_name) const;

//--------------------------------------------------------------------------

    // Return total lenght of text (including 0-terminators).
    TextPosition getLength() const;

    // Return lenght of i'th text (excluding 0-terminators).
    TextPosition getLength(DocId i) const;

    // Return C[c] + rank_c(L, i) for given c and i
    // Returns getLength() for invalid parameter values.
    TextPosition LF(uchar c, TextPosition i) const;

    // Given suffix i and substring length l, return T[SA[i] ... SA[i]+l-1] + '\0'.
    // Caller must delete [] the buffer.
    uchar* getSuffix(TextPosition pos, unsigned l) const;

    // For given suffix i, return corresponding DocId and text position.
    void getPosition(position_result& result, TextPosition i) const;

    // Enumerate document+position pairs (full_result) of
    // each suffix in given interval.
    void getPosition(position_vector& results, TextPosition sp, TextPosition ep) const;

//--------------------------------------------------------------------------

  private:
    const CSA::RLCSA* rlcsa;

    // These are not allowed.
    RLCSAWrapper();
    RLCSAWrapper(const RLCSAWrapper&);
    RLCSAWrapper& operator = (const RLCSAWrapper&);
};


inline TextCollection::position_result
convert(CSA::pair_type original)
{
  return TextCollection::position_result(original.first, original.second);
}


#endif  // RLCSA_WRAPPER_H
