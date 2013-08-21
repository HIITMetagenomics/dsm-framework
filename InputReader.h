/**
 * Handle input (file or stdin).
 *
 * Currently supported input formats:
 *   one-read-per-line (quality values in every second row, 
 *                      or in a separate file)
 *   FASTQ
 *   FASTA (quality values in a separate FASTA)
 *
 * If quality scores are stored in a separate file,
 * reads must occur in the same order in both files.
 *
 * TODO
 * [ ] Integer quality scores in FASTA
 * [ ] conversion between ASCII qualities
 */

#ifndef _InputReader_H_
#define _InputReader_H_

#include "Pattern.h"
#include <string>
#include <iostream>
#include <fstream>
#include <cstdlib> // exit()
#include <cassert>

/**
 * Base class to read input
 */
class InputReader
{
public:
    enum input_format_t { input_lines, input_fasta, input_fastq };

    static InputReader* build(input_format_t input, unsigned n, 
                              std::string file, std::string qual = "");

    /**
     * Retrieve the next pattern from input
     * Thread-safe.
     */      
    bool next(Pattern &p)
    {
        bool r = false;
#ifdef PARALLEL_SUPPORT
#pragma omp critical (INPUTREADER_NEXT)
#endif
        {
            if (!done())
            {
                assert(!n || n > count);
                assert(!this->eof());
                // Retrieve the next pattern, give it an id equal to curId:
                r = next(curId++, p);
                if (!r && n)
                    std::cerr << "unexpected end of file: unable to read the remaining " 
                              << n - count << " reads from input!" << std::endl;
                if (r)
                    ++count;
            }
        }
        return r;
    }

    bool skip(unsigned skip)
    { 
        unsigned i = 0;
        Pattern p;
        while (!done() && i < skip)
        {
            if (!this->next(curId, p))
                break;
            ++curId;
            ++i;
        }
        if (i != skip)
            return false;
        return true;
    }

    unsigned getCount()
    { return count; }

    virtual ~InputReader() { }

protected:
    // n_ == 0 to read until EOF
    InputReader(unsigned n_) 
        : curId(0), n(n_), count(0)
    { }


    // Done reading through the input?
    bool done() const
    {
        if (this->eof())
        {
            if (n)
                std::cerr << "Warning: input exhausted after " << curId << " reads, was expecting " 
                          << n - count << " more reads!" << std::endl;
            return true;
        }
        if (n && count >= n)
            return true;
        return false;
    }

    // Subclass provides an implementation:
    virtual bool eof() const = 0;
    virtual bool next(unsigned, Pattern &) = 0;

private:
    InputReader();
    // No copy constructor or assignment
    InputReader(InputReader const&);
    InputReader& operator = (InputReader const&);

    unsigned curId; // Current pattern id (0-base counter, includes skipped reads)
    unsigned n;     // Number of patterns to be processed (if 0, read until EOF)
    unsigned count; // Number of patterns processed so far (not including skipped)
};


/**
 * Format: One line per read.
 * Quality values in every second row, or in a separate file
 */
class SimpleLineInputReader : public InputReader
{
public:
    SimpleLineInputReader(unsigned n_, std::string file, std::string quality = "") 
        : InputReader(n_), 
        fp(0), fq(0)
    {
        // Open pattern file handle
        if (file == "-")
            fp = &std::cin;
        else
            fp = new std::ifstream(file.c_str());

        if (!fp->good())
        {
            std::cerr << "unable to read input file " << file << std::endl;
            exit(1); // or throw?
        }

        if (quality == "")
            return;

        // Open quality file handle
        if (file == quality)
            fq = fp;
        else if (quality == "-")
            fq = &std::cin;
        else
            fq = new std::ifstream(quality.c_str());

        if (!fq->good())
        {
            std::cerr << "unable to read input file " << quality << std::endl;
            std::exit(1); // or throw?
        }
    } 

    ~SimpleLineInputReader() 
    {
        if (fq != &std::cin)
            delete fq;
        fq = 0;

        if (fp != &std::cin)
            delete fp;
        fp = 0;
    }

    virtual bool eof() const
    {
        if (!fp || !fp->good()) // Pattern file
            return true;
        if (fq && !fq->good())  // Quality file
            return true;
        return false;
    }
    
/*    virtual unsigned skip(unsigned n)
    {
        unsigned i = 0;
        std::string row;
        // Skip n pattern rows
        while (i < n && std::getline(*fp, row).good())
            ++i;
        
        if (!fq->good() || i != n)
            return i;

        // Skip n quality rows
        i = 0;
        while (i < n && std::getline(*fq, row).good())
            ++i;
        return i;
        }*/

    virtual bool next(unsigned id, Pattern &p)
    {
        if (eof())
            return false;
        
        std::string row;
        if (!std::getline(*fp, row).good())
            return false;
        if (row.empty())
            return false;
        if (row[0] == '>')
        {
            std::cerr << "readligner: FASTA input detected, please check the input format and/or use option `-f, --fasta'" << std::endl;
            std::exit(1);
        }
        if (row[0] == '@')
        {
            std::cerr << "readligner: FASTQ input detected, please check the input format and/or use option `-q, --fastq'" << std::endl;
            std::exit(1);
        }
        
        if (fq == 0)
        {
            // No quality file            
            p = Pattern(id, row);
            return true;
        }
        
        std::string qual;
        if (!std::getline(*fq, qual).good())
            return false;
        
        p = Pattern(id, row, qual);
        return true;
    }


protected:
    std::istream *fp;
    std::istream *fq;

private:
    // No default constructor
    SimpleLineInputReader();
    // No copy constructor or assignment
    SimpleLineInputReader(SimpleLineInputReader const&);
    SimpleLineInputReader& operator = (SimpleLineInputReader const&);
};

/**
 * Format: FASTA input.
 * Quality values in a separate file
 */
class FastaInputReader : public SimpleLineInputReader
{
public:
    FastaInputReader(unsigned n_, std::string file, std::string quality = "") 
        : SimpleLineInputReader(n_, file, quality)
    { } 
    
/*    virtual unsigned skip(unsigned n)
    {
        unsigned i = 0;
        std::string row;
        // Skip n pattern rows
        while (i < n && std::getline(*fp, row).good())
            if (row[0] == '>') ++i;
        
        if (fq == 0 || i != n)
            return i;

        // Skip n quality rows
        i = 0;
        while (i < n && std::getline(*fq, row) != NULL)
            if (row[0] == '>') ++i;
        return i;
        }*/

    virtual bool next(unsigned id, Pattern &p)
    {
        if (this->eof())
            return false;
        
        std::string pat;
        std::string name;
        if (!FastaInputReader::readNextFasta(fp, pat, name))
            return false;
        
        if (!fq)
        {
            // No quality file
            p = Pattern(id, pat, "", name);
            return true;
        }
        
        std::string qual;
        std::string qname;
        if (!FastaInputReader::readNextFasta(fq, qual, qname))
            return false;
        
        if (name != qname)
            std::cerr << "Warning: fasta header of a read (" << name << ") did not match"
                      << " header in quality fasta (" << qname 
                      << "); reads and their quality values must occur in the same order!" << std::endl;
        
        p = Pattern(id, pat, qual, name);
        return true;
    }

private:

    static bool readNextFasta(std::istream *f, std::string &pat, std::string &name)
    {
        std::string row;
        while (std::getline(*f, row).good())
            if (row.empty() || row[0] == '>') break;
        if (!f->good())
            return false;
        if (row.empty())
            return false;

        // Trim any whitespace between '>' and name
        name = row.substr(row.find_first_not_of(" \t", 1));
        // End at first whitespace
        name = name.substr(0, name.find_first_of(" \t"));
            
        pat = "";
        while (std::getline(*f, row).good())
        {
            pat += row;

            // Peek the first symbol of the next row
            int c = f->peek();
            if ((char)c == '>' || c == EOF) break;
        }
        return true;
    }

    // No default constructor
    FastaInputReader();
    // No copy constructor or assignment
    FastaInputReader(FastaInputReader const&);
    FastaInputReader& operator = (FastaInputReader const&);
};

/**
 * Format: FASTQ
 */
class FastqInputReader : public InputReader
{
public:
    FastqInputReader(unsigned n_, std::string file) 
        : InputReader(n_), 
        fp(0)
    {
        // Open pattern file handle
        if (file == "-")
            fp = &std::cin;
        else
            fp = new std::ifstream(file.c_str());

        if (!fp->good())
        {
            std::cerr << "unable to read input file " << file << std::endl;
            exit(1); // or throw?
        }
    } 

    ~FastqInputReader() 
    {
        if (fp != &std::cin)
            delete fp;
        fp = 0;
    }

    virtual bool eof() const
    {
        if (!fp || !fp->good()) // Pattern file
            return true;
        return false;
    }
    
/*    virtual unsigned skip(unsigned n)
    {
        unsigned i = 0;
        std::string row;
        // Skip n pattern rows
        while (i < n && readNextFastq(row,row,row))
            ++i;

        if (i != n)
            std::cerr << "unable to read FASTQ input: expecting four rows per read, see README" << std::endl;
        
        return i;
        }*/

    virtual bool next(unsigned id, Pattern &p)
    {
        if (eof())
            return false;
        
        std::string name, pat, qual;
        if (!readNextFastq(pat, name, qual))
            return false;

        p = Pattern(id, pat, qual, name);
        return true;
    }

private:
    std::istream *fp;

    bool readNextFastq(std::string &pat, std::string &name, std::string &qual)
    {
        std::string row;
        if (!std::getline(*fp, row).good())
            return false;
        if (row.empty() || row[0] != '@') 
        {
            std::cerr << "unable to read FASTQ input: expecting four rows per read, see README" << std::endl;
            return false;
        }

        // Trim any whitespace between '@' and name
        name = row.substr(row.find_first_not_of(" \t", 1));
        // End at first '\t'
        name = name.substr(0, name.find_first_of(" \t"));

        if (!std::getline(*fp, pat).good())
            return false;

        if (!std::getline(*fp, row).good())
            return false;
        if (row.empty() || row[0] != '+') 
        {
            std::cerr << "unable to read FASTQ input: expecting four rows per read, see README" << std::endl;
            return false;
        }

        if (!std::getline(*fp, qual).good())
            return false;

        return true;
    }


    // No default constructor
    FastqInputReader();
    // No copy constructor or assignment
    FastqInputReader(FastqInputReader const&);
    FastqInputReader& operator = (FastqInputReader const&);
};


#endif // _InputReader_H_
