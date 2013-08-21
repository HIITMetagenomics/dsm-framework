#include "Pattern.h"

#include <sstream> // stringstream
#include <cassert>

bool Pattern::color = false;  // SOLiD color codes?

Pattern::Pattern(unsigned id_, std::string const & p_, 
                 std::string const & q_, 
                 std::string const & name_)
    : id(id_), p(p_), q(q_), name(name_), origq(q_),
      rc(false), truncated(0)
{ 
    if (name == "")
    {
        // Use id as a generic name
        std::stringstream out;
        out << id;
        name = out.str();
    }
        
    normalize(p); // Normalized to upper-case
    origp = p;
    
    if (!p.empty())
        truncate(0);  // Truncate color code

    assert(q == "" || q.size() == p.size());
}


void Pattern::truncate(std::size_t n)
{
    p = origp;
    q = origq;

    // Check adapter
    if (color && (p[0] == 'A' || p[0] == 'C' || p[0] == 'G' || p[0] == 'T'))
    {
        p = p.substr(2);
        if (q != "")
            p = q.substr(2);
    }

    if (2 * n >= p.size())
    {
        std::cerr << "Warning: truncated read " << name << " is empty!" << std::endl;
        p = "";
        return;
    }

    p = p.substr(n, p.size() - 2*n);
    if (q != "")
        q = q.substr(n, q.size() - 2*n);
    truncated = n;

    if (rc)
    {
        rc = false;
        this->reverseComplement();
    }
}

void Pattern::reverseComplement()
{
    rc = !rc;
    reverse(p);
    reverse(q);
    for (std::string::iterator it = p.begin(); it != p.end(); ++it)
        switch (*it) 
        {
        case('A'): *it = 'T'; break;
        case('C'): *it = 'G'; break;
        case('G'): *it = 'C'; break;
        case('T'): *it = 'A'; break;                
        }
}

void Pattern::normalize(std::string &t)
{
    bool valid = true;
    for (std::string::iterator it = t.begin(); it != t.end(); ++it)
    {
        switch (*it)
        {
        case('a'):
            *it = 'A';
            break;
        case('c'):
            *it = 'C';
            break;
        case('g'):
            *it = 'G';
            break;
        case('t'):
            *it = 'T';
            break;
        case('n'):
            *it = 'N';
            break;
        case('A'):
        case('C'):
        case('G'):
        case('T'):
        case('N'):
            break;
        case('0'):
        case('1'):
        case('2'):
        case('3'):
        case('.'):
            assert(color);                    
            break;
        default:
            *it = color ? '.' : 'N';
            valid = false;
            break;
        }
    }
    if (!valid)
        std::cerr << "Warning: read " << name << " contains invalid symbols: " << t << std::endl;
}
