#include "Query.h"

const char Query::ALPHABET_DNA[] = {'A', 'C', 'G', 'T'}; //, 'N'}; // Change alphabet_size also
const char Query::ALPHABET_SOLID[] = {'0', '1', '2', '3', '.'};


Query::Query(TextCollection *tc_, OutputWriter &ow, bool vrb, 
             bool incr, bool rvrs, unsigned rprt)
    : tc(tc_), outputw(ow), verbose(vrb), reverse(rvrs), debug(false), report(rprt),
      klimit(0), sum(0), textlen(tc->getLength()), rlimit(0), ALPHABET(ALPHABET_DNA),
      callcounter(0), counterstart(0), pathcounter(0)
{ 
    smin.push(0);
    smax.push(textlen-1);
    match.clear();
}


void Query::align(Pattern *p, unsigned k, unsigned &reported)
{
    this->p = p;
    this->pat = p->c_str();
    this->patlen = p->size();
    this->klimit = k;
    this->sum = reported;
    if (Pattern::getColor())
        ALPHABET = ALPHABET_SOLID;

    while (!smin.empty()) smin.pop();
    while (!smax.empty()) smax.pop();
    smin.push(0);
    smax.push(textlen-1);
    match.clear();
        
    firstStep();

    // Search reverse complemented?
    if (reverse && sum < report)
    {
        p->reverseComplement();
        this->pat = p->c_str();
        while (!smin.empty()) smin.pop();
        while (!smax.empty()) smax.pop();
        smin.push(0);
        smax.push(textlen-1);
        match.clear();
        
        firstStep();
    }

    reported = this->sum;
}
