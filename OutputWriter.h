/**
 * Handle output (file or stdin).
 *
 * Currently supported output formats:
 *   tab delimited (see README)
 *   SAM
 *
 * TODO
 * [ ] CIGAR
 * [ ] quality scores
 */

#ifndef _OutputWriter_H_
#define _OutputWriter_H_

#include "Pattern.h"
#include "TextCollection.h"

#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <cstdlib> // exit()
#include <cassert>

/**
 * Base class to write output
 */
class OutputWriter
{
public:
    enum output_format_t { output_tabs, output_sam };

    static OutputWriter* build(output_format_t output, std::string file = "");

    virtual void reportIndels(Pattern const &p, 
                        TextCollection::position_result const &pos, 
                        std::string const &refName,
                        std::string const &text,
                        unsigned occs) = 0;

    virtual void reportMismatches(Pattern const &p, 
                        TextCollection::position_result const &pos, 
                        std::string const &refName,
                        std::string const &text,
                        unsigned occs) = 0;

    virtual ~OutputWriter() 
    { 
        if (fp != &std::cout)
            delete fp;
        fp = 0;
    }


protected:
    OutputWriter(std::string const &file) 
        : fp(0)
    { 
        if (file == "")
            fp = &std::cout;
        else
            fp = new std::ofstream(file.c_str());

        if (!fp->good())
        {
            std::cerr << "unable to open output file " << file << std::endl;
            std::exit(1); // or throw?
        }
    }

    std::ostream *fp;
private:
    OutputWriter();
    // No copy constructor or assignment
    OutputWriter(OutputWriter const&);
    OutputWriter& operator = (OutputWriter const&);
};


/**
 * Format: tab delimited values, see README.
 * 
 * Inherit constructor from OutputWriter
 */
class TabDelimitedOutputWriter : public OutputWriter
{
public:
    TabDelimitedOutputWriter(std::string const &file) 
        : OutputWriter(file)
    { }

    virtual void reportIndels(Pattern const &p, 
                        TextCollection::position_result const &pos, 
                        std::string const &refName,
                        std::string const &text,
                        unsigned occs)
    {
        std::string edits = getEdits(p.getPattern(), text);
        report(p, pos, refName, text, occs, edits);
    }

    virtual void reportMismatches(Pattern const &p, 
                        TextCollection::position_result const &pos, 
                        std::string const &refName,
                        std::string const &text,
                        unsigned occs)
    {
        std::string edits = getMismatches(p.getPattern(), text);
        report(p, pos, refName, text, occs, edits);
    }

private:
    inline void report(Pattern const &p, 
                       TextCollection::position_result const &pos, 
                       std::string const &refName,
                       std::string const &text,
                       unsigned occs,
                       std::string const &edits)
    {
#ifdef PARALLEL_SUPPORT
#pragma omp critical (OUTPUTWRITER_REPORT)
#endif
        {
            if (!fp->good())
            {
                std::cerr << "error: unable to write output!" << std::endl;
                std::abort();
            }
            *fp << p.getName() << "\t" 
                << refName << "\t" 
                << pos.second+1 << "\t"
                << pos.second+text.size() << "\t"
                << occs << "\t"
                << (p.isReverseComplement() ? 'R' : 'F') << "\t"
                << edits << std::endl;
        }
    }

    std::string getEdits(std::string const &, std::string const &);
    std::string getMismatches(std::string const &, std::string const &);
};

/**
 * Format: SAM
 *
 * Inherit constructor from OutputWriter
 */
class SamOutputWriter : public OutputWriter
{
public:
    SamOutputWriter(std::string const &file) 
        : OutputWriter(file)
    { }

    virtual void reportIndels(Pattern const &p, 
                        TextCollection::position_result const &pos, 
                        std::string const &refName,
                        std::string const &text,
                        unsigned occs)
    {
        std::string cigar = getCigar(p.getPattern(), text);        
        report(p, pos, refName, text, occs, cigar);
    }

    virtual void reportMismatches(Pattern const &p, 
                        TextCollection::position_result const &pos, 
                        std::string const &refName,
                        std::string const &text,
                        unsigned occs)
    {
        std::stringstream cigar;
        cigar << p.size();
        cigar << "M";
        report(p, pos, refName, text, occs, cigar.str());
    }

private:
    inline void report(Pattern const &p, 
                       TextCollection::position_result const &pos, 
                       std::string const &refName,
                       std::string const &text,
                       unsigned occs,
                       std::string const &cigar)
    {
#ifdef PARALLEL_SUPPORT
#pragma omp critical (OUTPUTWRITER_REPORT)
#endif
        {
            if (!fp->good())
            {
                std::cerr << "error: unable to write output!" << std::endl;
                std::abort();
            }
            *fp << p.getName() << '\t';    // QNAME
            unsigned flag = 0;             // FLAG
            if (p.isReverseComplement())
                flag |= 0x0010;            // strand of the query (0 forward, 1 reverse)
            
            *fp << flag << '\t'
                << refName << '\t'         // RNAME 
                << pos.second+1 << '\t'    // POS
                << 255 << '\t'             // MAPQ (MAPping Quality)
                << cigar << '\t'           
                << "*\t"                   // MRNM (Mate Reference sequence NaMe)
                << "0\t"                   // MPOS (Mate POSition)
                << "0\t";                  // ISIZE (inferred Insert SIZE)
            if (Pattern::getColor())       // SEQ (query SEQuence)
                *fp << "*\t";
            else
                *fp << p.getPattern() << '\t';

            /*if (p.getQuality().empty())    // QUAL (query QUALity)
                *fp << "*\t";
            else
            {   FIXME Quality should be in PHRED33 format
                assert(p.getQuality().size() == p.getPattern().size());
                *fp << p.getQuality() << '\t';
                }*/
            *fp << "*\t";

            // Optional fields

            *fp << std::endl;
        }
    }
    std::string getCigar(std::string const &, std::string const &);
};

#endif // _OutputWriter_H_
