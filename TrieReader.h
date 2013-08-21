/**
 * Parse input from socket.
 *
 * Currently supported input formats:
 *    Trie
 */

#ifndef _TrieReader_H_
#define _TrieReader_H_

#include "Tools.h"
#include "ServerSocket.h"

#include <sstream>
#include <string>
#include <iostream>
#include <cstdlib> // exit()
#include <cassert>


/**
 * Base class to read input
 */
class TrieReader
{
public:
    TrieReader(int i, std::string name, ServerSocket *ssocket, bool vrbs, bool dbg)
        : id(i), filename(name), ifs(ssocket), verbose(vrbs), debug(dbg), rate(time(NULL)), n(0), occs(0)
    { }

    bool hasChild()
    {
        if (!ifs->good())
        {
            if (debug) std::cerr << "ifstream exausted normally at " << filename << std::endl;
            return false;
        }
        char c = ifs->peek();
        if (!ifs->good())
        {
            if (debug) std::cerr << "ifstream exausted normally at " << filename << std::endl;
            return false;
        }

        if (c == '(')
            return true;
        return false;
    }
    char readChild()
    {
        if (n % 1000000 == 0)
            rate = std::time(NULL); // Reset timer

        char c = ifs->getc();
        if (c != '(')
            perror(std::string("expecting ( byte but got ") + c);
        c = ifs->getc();
        if (!isDna(c))
        {
            ifs->debug();
            perror(std::string("expecting dna byte but got ") + c);
        }
        ++n;
        return c;
    }

    ulong readOccs()
    {
        occs = ifs->getulong();
        return occs;
    }

    void readClose()
    {
        char c = ifs->getc();
        if (c != ')')
            perror(std::string("expecting ) byte but got ") + c);
    }


    void checkR()
    {
        char c = ifs->getc();
        if (c != 'R')
        {
            std::cerr << "expecting R byte but got " << c;
            for (int i = 0; i < 10 && ifs->good(); ++i)
                std::cerr << (char)ifs->getc();
            std::cerr << std::endl << " at file " << filename << std::endl;
            if (!ifs->good())
                std::cerr << "error: ifs was not good" << std::endl;
            std::exit(1);
        }
        ulong checksum = ifs->getulong();
        //if (debug) std::cerr << " checksum = " << checksum << ", total occs = " 
        //                    << n << "in reader " << filename << std::endl;
        if (checksum != n)
        {
            std::cerr << "error: total number traversed = " << n << " but checksum was " 
                      << checksum << " (" << filename << ")" << std::endl;
            std::exit(1);
        }
    }

    bool good()
    {
        return ifs->good();
    }

    ulong getOccs()
    { return occs; }
    int getId()
    { return id; }
    std::string getName()
    { return filename; }

    virtual ~TrieReader() 
    { 
        delete ifs;
    }

    // Check whether or not we have successfully exausted all of input
    void checkEof()
    {
        if (!ifs->good())
        {
            if (debug) std::cerr << "ifs " << id << " was exausted normally." << std::endl;
            return;
        }

        std::cerr << std::endl << "WARNING: Something is wrong... more input pending at " << filename << std::endl
                  << "Next ten bytes are (quoted): \"";
        for (int i = 0; i < 10 && ifs->good(); ++i)
            std::cerr << (char) ifs->getc();
        std::cerr << "\"" << std::endl;
        if (ifs->good())
            std::cerr << "ifs is still good" << std::endl;
        else
            std::cerr << "ifs is not good anymore!" << std::endl;
    }

    double getRate()
    {
        return std::difftime(time(NULL), rate);
    }

    /**
     * Sends message to client and requests to stop the current branch
     * FIXME requires checksum
     */
    void sendHalt(ulong depth)
    {
        ifs->writeHalt(n, depth);
    }

protected:

    /**
     * Extracts an ulong from ifs
     *
     * If there is no number to extract, return 1.
     */
/*    ulong readUlong()
    {
        
        std::string str;
        char c = ifs->get();
        while (ifs->good() && isNumber(c))
        {
            str += c;            
            c = ifs->get();
        }
        if (ifs->good())
            ifs->putback(c); // Restore the next non-number

        if (str.empty())
        {
            std::cerr << "TrieReader::readUlong(): unable to read ulong from " << filename << std::endl;
            if (!ifs->good())
                std::cerr << "ifs was not good" << std::endl;
            std::cerr << "debug: ";
            for (int i = 0; i < 10 && ifs->good(); ++i)
                std::cerr << (char) ifs->get();
            std::cerr << std::endl;
            if (!ifs->good())
                std::cerr << "ifs was not good" << std::endl;
            std::cerr << "TrieReader::readUlong(): unable to read ulong from " << filename << " after n = " << n << std::endl;
            std::exit(1);
        }
        ulong value;
        std::stringstream(str) >> value;
        return value;
        }*/

    bool isNumber(char c)
    {
        if (c >= '0' && c <= '9')
            return true;
        return false;
    }

    bool isDna(char c)
    {
        if (c == 'A' || c == 'C' || c == 'G' || c == 'T' || c == 'N')
            return true;
        return false;
    }

    void perror(std::string str)
    {
        std::cerr << std::endl << "error: " << str << " at reader " << filename << std::endl;
        std::exit(1);
    }

    int id;
    std::string filename;
    ServerSocket *ifs;
    bool verbose;
    bool debug;
    time_t rate;
    ulong n;    
    ulong occs;
private:
    TrieReader();
    // No copy constructor and assignment
    TrieReader(TrieReader const&);
    TrieReader& operator = (TrieReader const&);

};

#endif // _TrieReader_H_
