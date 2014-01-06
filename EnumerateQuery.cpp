#include "EnumerateQuery.h"
#include "omp.h"

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

    for (unsigned i = 0; i < ALPHABET_SIZE; ++i)
    {
        while (!extmin[i].empty()) extmin[i].pop();
        while (!extmax[i].empty()) extmax[i].pop();
        extmin[i].push(tc->LF(ALPHABET[i], (ulong)-1));
        extmax[i].push(tc->LF(ALPHABET[i], textlen-1)-1);
    }

    wctime = time(NULL);
    wcrate = time(NULL);

    //nodeids.reserve(1024*1024);
    
    if (enforcepath.empty())
        nextSymbol();
    else
        nextEnforced();

    reported = (unsigned) this->reported;
}

bool EnumerateQuery::pushChar(char c)
{
    if (!Query::pushChar(c))
        return false;
    // Query::pushChar was successful, update the extended intervals
    for (unsigned i = 0; i < ALPHABET_SIZE; ++i)
    {
        ulong nmin = extmin[i].top();
        ulong nmax = extmax[i].top();
        if (nmin <= nmax)
        {
            nmin = tc->LF(c, nmin-1);
            nmax = tc->LF(c, nmax)-1;
        }
        extmin[i].push(nmin); // Push always to keep stacks in sync
        extmax[i].push(nmax);
    }

    return true;
}

void EnumerateQuery::popChar()
{
    Query::popChar();
    for (unsigned i = 0; i < ALPHABET_SIZE; ++i)
    {
        extmin[i].pop();
        extmax[i].pop();
    }
}

/**
 * Returns the left-char symbol
 *   '0' iff all left-chars are '\0' or 'N'
 *   'N' iff there are multiple left-chars
 *   'A', 'C', 'G', 'T'
 *       iff the left-char is one of ACGT.
 */
char EnumerateQuery::leftChar()
{
    bool matches = false; // If any extended interval matches P
    bool any = false;     // If any extended interval exists (A,C,G,T)
    unsigned c = 255;         // Left char
    ulong sum = 0;        // For debug

    for (unsigned i = 0; i < ALPHABET_SIZE; ++i)
    {
        ulong nmin = extmin[i].top();
        ulong nmax = extmax[i].top();
        if (nmin <= nmax)
        {
            sum += nmax-nmin+1;
            any = true;
            c = i;
            if (nmin == smin.top() && nmax == smax.top())
                matches = true;
        }
    }
    assert(sum <= smax.top()-smin.top()+1);
    if (matches)
        return ALPHABET[c];
    if (any)
        return 'N';
    return '0';
}

void EnumerateQuery::followOneBranch()
{
    unsigned i = 0;
    uchar c = tc->getL(smin.top());    
    while (c == 'A' || c == 'C' || c == 'G' || c == 'T') // FIXME && match.size() < maxdepth)
    {        
        /*if (i % 16 == 0 && (haltTo = cs->checkHalt(haltDepth)))
        {
            if (nodeids.size() <= haltDepth || nodeids[haltDepth] != haltTo)
            {
                haltTo = 0;
                haltDepth = ~0u;
            }
            break;
            }*/
        if (match.size() >= maxdepth)
            break;

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
        cs->putc(leftChar());
        cs->putc(')');
        popChar();
    }
}

void EnumerateQuery::nextSymbol()
{
    if (match.size() >= maxdepth)
        return;

    if (smax.top() - smin.top() == 0)
    {
        followOneBranch();
        return;
    }
    
/*    nodeids.push_back(reported);
        
    if (match.size() % 16 == 0 && !haltTo)
    {
        haltTo = cs->checkHalt(haltDepth);
        if (haltTo)
        {
            //std::cerr << "received message: " << haltTo << ", depth " << haltDepth << std::endl;
            //if (nodeids.size() > haltDepth+1)
            //    std::cerr << nodeids[haltDepth-1] << ", " <<nodeids[haltDepth] << ", " << nodeids[haltDepth+1] << std::endl;

            if (nodeids.size() > haltDepth && nodeids[haltDepth] == haltTo)
            {
                nodeids.pop_back();
                return;
            }
            haltTo = 0;
            haltDepth = ~0u;
        }
        }*/

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
#pragma omp critical (CERR_OUTPUT)
{
            int tnum = omp_get_thread_num();
            std::cerr << tnum << ": ";
            for (std::vector<uchar>::iterator it = match.begin(); it != match.end(); ++it)
                std::cerr << *it;
            std::cerr << ' ' << smax.top() - smin.top() + 1 << " (" 
                      << reported << " reported, " 
                      << std::difftime(time(NULL), wctime) << " seconds, " 
                      << std::difftime(time(NULL), wctime) / 3600 << " hours)" << std::endl;
}
        }

        cs->putc('(');
        cs->putc(match[match.size()-1]);
        ++reported;        
        nextSymbol();

//        if (smax.top() - smin.top() > 0)
        cs->putulong(smax.top() - smin.top() + 1);
        if (match.size() <= 6)
        {
            cs->putc('R');
            cs->putulong(reported);
        }
      
        cs->putc(leftChar());
        cs->putc(')');
	popChar();
    }

             /*if (haltDepth == match.size())
    {
        if (nodeids.back() != haltTo)
        { // Sanity check
            std::cerr << "nodeids.back() != haltTo" << std::endl;
            std::exit(1);
        }
        
        haltDepth = ~0u;
        haltTo = 0;
    }    

    nodeids.pop_back();*/
}

void EnumerateQuery::nextEnforced()
{
    callcounter++;

    //nodeids.push_back(reported);

    char c = enforcepath[match.size()];
    if (!pushChar(c)) return;
    if (smax.top() - smin.top() + 1 < fmin)
    {
        popChar();
        return; 
    }

    if (match.size() <= 5)
    {
#pragma omp critical (CERR_OUTPUT)
{
        int tnum = omp_get_thread_num();
        std::cerr << tnum << ": Enforced path: ";
        for (std::vector<uchar>::iterator it = match.begin(); it != match.end(); ++it)
            std::cerr << *it;
        std::cerr << ' ' << smax.top() - smin.top() + 1 << " (" 
                  << reported << " reported, " 
                  << std::difftime(time(NULL), wctime) << " seconds, " 
                  << std::difftime(time(NULL), wctime) / 3600 << " hours)" << std::endl;
}
    }

    cs->putc('(');
    cs->putc(match[match.size()-1]);
    ++reported;
    
    if (enforcepath.size() > match.size())
        nextEnforced();
    else
        nextSymbol();

//        if (smax.top() - smin.top() > 0)
    cs->putulong(smax.top() - smin.top() + 1);
    if (match.size() <= 6)
    {
        cs->putc('R');
        cs->putulong(reported);
    }
    
    cs->putc(leftChar());
    cs->putc(')');
    popChar();
    //nodeids.pop_back();
}

