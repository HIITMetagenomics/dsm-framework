#include "EnumerateQuery.h"

void EnumerateQuery::firstStep()
{
    std::cerr << "Error: firststep called!" << std::endl;
}

void EnumerateQuery::enumerate(unsigned &reported)
{
    match.clear();
    while (!smin.empty()) smin.pop();
    while (!smax.empty()) smax.pop();
    
    smin.push(0);
    smax.push(textlen-1);

    wctime = time(NULL);
    wcrate = time(NULL);
    if (enforcepath.empty())
        nextSymbol();
    else
        nextEnforced();

    reported = (unsigned) this->reported;
}

void EnumerateQuery::followOneBranch()
{
    unsigned i = 0;
    uchar c = tc->getL(smin.top());    
    while (c == 'A' || c == 'C' || c == 'G' || c == 'T') // FIXME && match.size() < maxdepth)
    {        
        ++i;
        if (!pushChar(c)) 
        {
            std::cerr << "EnumQuery::followOneBranch(): pushchar failed!?" << std::endl;
            std::exit(1);
        }
        
        cs->putc('(');
        cs->putc(c);
        ++reported;
        c = tc->getL(smin.top()); 
    }

    while (i > 0)
    {
        --i;
        cs->putulong(1); // Freq.
        if (match.size() <= 6)
        {
            cs->putc('R');
            cs->putulong(reported);
        }
        cs->putc(')');
        popChar();
    }
}

void EnumerateQuery::nextSymbol()
{
    if (smax.top() - smin.top() == 0)
    {
        followOneBranch();
        return;
    }
    // ALPHABET has been defined in BTSearch
    for (const char *c = ALPHABET; c < ALPHABET + ALPHABET_SIZE; ++c) {  /*&& haltDepth > match.size()*/
	if (!pushChar(*c)) continue;
        if (smax.top() - smin.top() + 1 < fmin)
        {
            popChar();
            continue; 
        }

        if (match.size() <= 5)
        {
            for (std::vector<uchar>::iterator it = match.begin(); it != match.end(); ++it)
                std::cerr << *it;
            std::cerr << ' ' << smax.top() - smin.top() + 1 << " (" 
                      << reported << " reported, " 
                      << std::difftime(time(NULL), wctime) << " seconds, " 
                      << std::difftime(time(NULL), wctime) / 3600 << " hours)" << std::endl;
        }

        cs->putc('(');
        cs->putc(match[match.size()-1]);
        ++reported;        
        nextSymbol();

        cs->putulong(smax.top() - smin.top() + 1);
        if (match.size() <= 6)
        {
            cs->putc('R');
            cs->putulong(reported);
        }
      
        cs->putc(')');
	popChar();
    }
}

void EnumerateQuery::nextEnforced()
{
    callcounter++;
    char c = enforcepath[match.size()];
    if (!pushChar(c)) return;
    if (smax.top() - smin.top() + 1 < fmin)
    {
        popChar();
        return; 
    }

    if (match.size() <= 5)
    {
        std::cerr << "Enforced path: ";
        for (std::vector<uchar>::iterator it = match.begin(); it != match.end(); ++it)
            std::cerr << *it;
        std::cerr << ' ' << smax.top() - smin.top() + 1 << " (" 
                  << reported << " reported, " 
                  << std::difftime(time(NULL), wctime) << " seconds, " 
                  << std::difftime(time(NULL), wctime) / 3600 << " hours)" << std::endl;
    }

    cs->putc('(');
    cs->putc(match[match.size()-1]);
    ++reported;
    
    if (enforcepath.size() > match.size())
        nextEnforced();
    else
        nextSymbol();

    cs->putulong(smax.top() - smin.top() + 1);
    if (match.size() <= 6)
    {
        cs->putc('R');
        cs->putulong(reported);
    }
    
    cs->putc(')');
    popChar();
}

