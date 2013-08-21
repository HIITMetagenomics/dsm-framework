/**
 * Model for patterns/reads
 *
 * TODO:
 *
 *  [ ] Set bool color;
 *  [ ] 
 */

#ifndef _Pattern_H_
#define _Pattern_H_

#include "Tools.h"

#include <string>

class Pattern
{
public:
    Pattern(unsigned id_ = 0, std::string const & = "", 
            std::string const & = "", std::string const & = "");

    inline unsigned getId() const
    { return id; }
    inline std::string const & getName() const
    { return name; }

    // Access pattern
    inline unsigned size() const
    { return p.size(); }
    inline char const * c_str() const
    { return p.c_str(); }
    inline std::string const & getPattern() const
    { return p; }
    inline std::string const & getOrigPattern() const
    { return origp; }


    // Access quality values
    inline std::string const & getQuality() const
    { return q; }
    inline std::string const & getOrigQuality() const
    { return origq; }

    inline bool isReversed() const
    { return reversed; }
    inline bool isReverseComplement() const
    { return rc; }

    static void setColor(bool b)
    { color = b; }
    static bool getColor()
    { return color; }

    void truncate(std::size_t);
    inline void reverse()
    {
        reversed = !reversed;
        reverse(p);
        reverse(q);
    }
    void reverseComplement();
    
private:
    static bool color;  // SOLiD color codes?

    unsigned id;
    std::string p;  // Truncated and/or rev.complemented pattern 
    std::string q;  // Truncated and/or rev.complemented quality values
    std::string name;  // Unique name tag
    std::string origp; // Original pattern
    std::string origq; // Original quality values
    bool reversed;     // Pattern is reversed
    bool rc;           // Reverse complemented
    unsigned truncated;// Number of truncated bases

    inline void reverse(std::string &t)
    {
        char c;
        std::size_t n = t.size();
        for (std::size_t i = 0; i < n / 2; ++i) {
            c = t[i];
            t[i] = t[n - i - 1];
            t[n - i - 1] = c;
        }
    }

    /**
     * Normalize to upper-case
     */
    void normalize(std::string &);
    
};

#endif // _Pattern_H_
