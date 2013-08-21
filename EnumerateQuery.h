#ifndef _EnumerateQuery_H_
#define _EnumerateQuery_H_

#include "Query.h"
#include "ClientSocket.h"

#include <string>
#include <cstdlib> // exit()
#include <ctime>
#include <vector>

class EnumerateQuery : public Query
{
public:
EnumerateQuery(TextCollection *tc, OutputWriter &ow, bool vrb, ClientSocket *csocket, std::string const &ep, unsigned fm, unsigned md)
    : Query(tc, ow, vrb, 0, 0, 0), reported(0), cs(csocket), enforcepath(ep), fmin(fm), maxdepth(md) //, haltTo(0), haltDepth(~0u)
    { }

    virtual ~EnumerateQuery()
    {
        delete cs;
    }

    
    void enumerate(unsigned &reported);
protected:
    void firstStep();

private:
    ulong reported;
    ClientSocket *cs;
    time_t wctime;
    time_t wcrate;
    std::string const &enforcepath;

    TextCollection *tcr; // Index for reversed text
    unsigned fmin; 
    unsigned maxdepth;

    void followOneBranch();
    void nextSymbol();
    void nextEnforced();
   
};

#endif // _EnumerateQuery_H_
