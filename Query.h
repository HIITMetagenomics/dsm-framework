/**
 * Interface for alignment queries
 */

#ifndef _Query_H_
#define _Query_H_

#include "Pattern.h"
#include "InputReader.h"
#include "OutputWriter.h"

#include <stack>
#include <vector>
#include <string>
#include <cstdlib> // exit()

/**
 * Base class for queries
 */
class Query
{
public:
    void align(Pattern *p, unsigned k, unsigned &reported);

    void setRecursionLimit(unsigned rlimit)
    { this->rlimit = rlimit; }

    void setDebug(bool b)
    { this->debug = b; }

    virtual ~Query() 
    { }
 
    static const char ALPHABET_DNA[];
    static const unsigned ALPHABET_SIZE = 4; // 5
 
    inline bool pushChar(char c) {
        ulong nmin = tc->LF(c, smin.top()-1);
        ulong nmax = tc->LF(c, smax.top())-1;
        if (nmin > nmax) return false;
        smin.push(nmin);
        smax.push(nmax);
        match.push_back(c);
        return true;
    }

    inline void popChar() {
        smin.pop();
        smax.pop();
        match.pop_back();
    }
protected:
    virtual void firstStep() = 0;

    inline void pushDelim(char c) {
        match.push_back(c);
    }

    inline void popDelim() {
        match.pop_back();
    }

    inline int min(int a, int b) {
        if (a < b) return a; else return b;
    }

    inline int min(int a, int b, int c) {
        if (a < b) if (a < c) return a; else return c;
        else if (b < c) return b; else return c;
    }

    /*inline void newMatch(void) {
        ulong min = smin.top();
        ulong occs = smax.top() - min + 1;
        if (occs > report - sum)
            occs = report - sum;
        this->sum += occs;

        TextCollection::position_vector posv;
        tc.getPosition(posv, min, min + occs - 1);

        for (TextCollection::position_vector::iterator pos = posv.begin(); 
             pos != posv.end(); ++pos) 
            outputw.report(*p, *pos, "RNAME", std::string(text, l), 
                           smax.top() - min + 1); // FIXME
    }*/


    Query(TextCollection *tc_, OutputWriter &ow, bool vrb, 
          bool incr, bool rvrs, unsigned rprt);

    Pattern *p;
    unsigned patlen;
    const char *pat;
    TextCollection *tc;
    OutputWriter &outputw;
    bool verbose;
    bool reverse;
    bool debug;
    unsigned report;
    unsigned klimit;
    unsigned sum;
    ulong textlen;
    unsigned rlimit;
    
    const char *ALPHABET;
    static const char ALPHABET_SOLID[];
    static const char ALPHABET_SHIFTED[];

    std::stack<ulong> smin;
    std::stack<ulong> smax;
    std::vector<uchar> match;

    ulong callcounter;
    ulong counterstart;
    ulong pathcounter; // FIXME

private:
    Query();
    // No copy constructor or assignment
    Query(Query const&);
    Query& operator = (Query const&);
};

#endif // _Query_H_
