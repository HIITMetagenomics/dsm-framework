#include "OutputWriter.h"
#include <sstream>
#include <vector>
using std::vector;
#include <string>
using std::string;
#include <cctype>
#include <cstdio>
#include <cassert>

/**
 * TODO
 * [ ] faster edit distance computation
 */

OutputWriter* OutputWriter::build(output_format_t output, std::string file)
{
    switch (output) 
    {
    case output_tabs:
        return new TabDelimitedOutputWriter(file);
        break;
    case output_sam:
        return new SamOutputWriter(file);
        break;
    default:
        std::abort();
    }
}

/**
 * Simple (and slow) edit distance computation
 */
typedef struct matrix
{
    static const char DIR_NORTH = '^';
    static const char DIR_WEST = '<';
    static const char DIR_NORTHWEST = '\\';

    int value;
    char direction;

    matrix()
        : value(-1), direction('0')
    { }
} dmatrix;

dmatrix *** initMatrix(string const &p, string const &t)
{
    const int m = p.size();
    const int n = t.size();
    
    dmatrix ***d = new dmatrix ** [n+1];
    for (int i = 0; i <= n; ++i)
    {
        d[i] = new dmatrix * [m+1];
        for (int j = 0; j <= m; ++j)
            d[i][j] = new dmatrix;
    }

    for (int i = 0; i <= m; ++i)
    {
        d[0][i]->value = i;
        d[0][i]->direction = dmatrix::DIR_NORTH;
    }
    for (int i = 0; i <= n; ++i)
    {
        d[i][0]->value = i;
        d[i][0]->direction = dmatrix::DIR_WEST;
    }

    for (int i = 1; i <= n; ++i)
        for (int j = 1; j <= m; ++j)
        {
            int north = d[i][j-1]->value + 1;
            int west = d[i-1][j]->value + 1;
            int nw = d[i-1][j-1]->value;
            if (p[j-1] != t[i-1])
                ++ nw;
            
            if (nw <= north && nw <= west)
            {
                d[i][j]->value = nw;
                d[i][j]->direction = dmatrix::DIR_NORTHWEST;
            }
            else if (north <= west)
            {
                d[i][j]->value = north;
                d[i][j]->direction = dmatrix::DIR_NORTH;
            }
            else
            {
                d[i][j]->value = west;
                d[i][j]->direction = dmatrix::DIR_WEST;
            }
            assert(d[i][j]->value != -1);
            assert(d[i][j]->direction != '0');
        }
    return d;
}

void freeMatrix(dmatrix ***d, int m, int n)
{
    for (int i = 0; i < n+1; ++i)
    {
        for (int j = 0; j < m+1; ++j)
            delete d[i][j];
        delete [] d[i];
    }
    delete [] d;
}

std::string TabDelimitedOutputWriter::getEdits(std::string const &p, std::string const &t)
{
    const int m = p.size();
    const int n = t.size();
    dmatrix ***d = initMatrix(p, t);

    vector<string> result;
    char tmp[50];
    int i = n, j = m;
    int check = d[n][m]->value;
    if (check != 0)
        while (i != 0 || j != 0)
        {
            switch (d[i][j]->direction)
            {
            case(dmatrix::DIR_NORTHWEST):
                if (p[j-1] != t[i-1])
                {
                    --check;
                    snprintf(tmp, 50, " %d %c", i-1, p[j-1]);
                    result.push_back(string(tmp));
                }
                --j;
                --i;
                break;

            case(dmatrix::DIR_NORTH):
                --check;
                snprintf(tmp, 50, " %d %c", i, std::tolower(p[j-1]));
                result.push_back(string(tmp));
                --j;
                break;
            case(dmatrix::DIR_WEST):
                --check;
                snprintf(tmp, 50, " %d D", i-1);
                result.push_back(string(tmp));
                --i;
                break;
            
            }
        }
    assert(check == 0);

    freeMatrix(d, m, n);

    if (result.empty())
        return "";
    
    string edits;
    for (vector<string>::reverse_iterator it = result.rbegin(); it != result.rend(); ++it)
        edits += *it;

    return edits.substr(1);
}

std::string TabDelimitedOutputWriter::getMismatches(std::string const &pat, std::string const &text)
{
    if (text.size() != pat.size())
    {
        std::cerr << "OutputWriter::getMismatches: assert failed" << std::endl;
        abort();
    }
    unsigned i = 0;
    while (i < pat.size() && text[i] == pat[i])
        ++i;
    if (i >= pat.size())
        return "";

    std::stringstream out;
    out << i << " " << pat[i]; // First mismatch
    for (++i; i < pat.size(); ++i)
        if (text[i] != pat[i])
            out << " " << i << " " << pat[i];
    return out.str();
}


// TODO clean up
std::string SamOutputWriter::getCigar(std::string const &p, std::string const &t)
{
    const int m = p.size();
    const int n = t.size();
    dmatrix ***d = initMatrix(p, t);

    vector<string> result;
    char tmp[50];
    int i = n, j = m;
    int check = d[n][m]->value;
    int run = 0;     // number of times subsequent edit operation occurs
    char runc = 'M'; // edit operation CIGAR character
    if (check != 0)
        while (i != 0 || j != 0)
        {
            switch (d[i][j]->direction)
            {
            case(dmatrix::DIR_NORTHWEST):
                if (p[j-1] != t[i-1])
                    --check;
                --j;
                --i;
                if (run && runc != 'M')
                {
                    snprintf(tmp, 50, "%d%c", run, runc);
                    result.push_back(string(tmp));
                    run = 0;
                }
                ++run;
                runc = 'M';
                break;

            case(dmatrix::DIR_NORTH):
                --check;
                if (run && runc != 'I')
                {
                    snprintf(tmp, 50, "%d%c", run, runc);
                    result.push_back(string(tmp));
                    run = 0;
                }
                ++run;
                runc = 'I';
                --j;
                break;
            case(dmatrix::DIR_WEST):
                --check;
                if (run && runc != 'D')
                {
                    snprintf(tmp, 50, "%d%c", run, runc);
                    result.push_back(string(tmp));
                    run = 0;
                }
                ++run;
                runc = 'D';
                --i;
                break;
            
            }
        }
    else
    {
        // edit distance == 0
        run = m;
        runc = 'M';
    }
    assert(check == 0);
    assert(run);
    // Flush buffer
    sprintf(tmp, "%d%c", run, runc);
    result.push_back(string(tmp));

    freeMatrix(d, m, n);

    string edits;
    for (vector<string>::reverse_iterator it = result.rbegin(); it != result.rend(); ++it)
        edits += *it;

    return edits;
}
