#include "TextCollectionBuilder.h"

#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <ctime>
#include <cstring>
#include <cassert>
#include <getopt.h>

using namespace std;

/**
 * Flags set based on command line parameters
 */
bool verbose = false;
bool color = false;      // Convert DNA to color codes?
bool rotation = false;   // Build rotation index?
unsigned patlen = 0;     // pattern length for rotation index


void revstr(std::string &t)
{
    char c;
    std::size_t n = t.size();
    for (std::size_t i = 0; i < n / 2; ++i) {
        c = t[i];
        t[i] = t[n - i - 1];
        t[n - i - 1] = c;
    }
}

void complement(std::string &t)
{
    for (std::string::iterator it = t.begin(); it != t.end(); ++it)
    {
        switch (*it)
        {
        case('T'):
            *it = 'A';
            break;
        case('G'):
            *it = 'C';
            break;
        case('C'):
            *it = 'G';
            break;
        case('A'):
            *it = 'T';
            break;
        }
    }
}

/**
 * Normalize to upper-case
 */
void normalize(std::string &t, string const &name)
{
    string invalidSymbols = "";
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
            break;
        default:
            size_t pos = invalidSymbols.find_first_of(*it);
            if (pos == string::npos)
                invalidSymbols += *it;
            *it = 'N';
            break;
        }
    }
    if (invalidSymbols != "")
        std::cerr << "Warning: sequence " << name << " contains invalid symbol(s): " << invalidSymbols << std::endl;
}

bool is_color_bases(string const &text)
{
    string::const_iterator it = text.begin();
    ++it; // Skip first (possibly an adaptor symbol)
    for (;it != text.end(); ++it)
        if (*it != '0' && *it != '1' && *it != '2' && *it != '3' && *it != '.')
            return false;
    return true;            
}

bool is_dna_bases(string const &text)
{
    for (string::const_iterator it = text.begin();it != text.end(); ++it)
        if (*it != 'A' && *it != 'C' && *it != 'G' && *it != 'T' && *it != 'N')
            return false;
    return true;
}

void dna_to_color(string &text, string const &name)
{
    string output = "";
    string::const_iterator it = text.begin();
    
    if (is_color_bases(text))
    {
        if (verbose)
            cerr << "sequence " << name << " already in color bases, skipping conversion" << endl;
        return;
    }

    // We have to assume DNA alphabet from here on (no mixed solid/dna symbols)
    if (!is_dna_bases(text))
    {
        cerr << "error: sequence " << name << " contains color (0123.) and DNA bases (ACGTN) mixed together!" << endl;
        abort();
    }

    char code[5][5] = {{'0', '1', '2', '3', '.'},
                       {'1', '0', '3', '2', '.'},
                       {'2', '3', '0', '1', '.'},
                       {'3', '2', '1', '0', '.'},
                       {'.', '.', '.', '.', '.'}};

    int map[256];
    for (unsigned i = 0; i < 256; ++i)
        map[i] = -1;
    map[(int)'A'] = 0;
    map[(int)'C'] = 1;
    map[(int)'G'] = 2;
    map[(int)'T'] = 3;
    map[(int)'N'] = 4;

    int prev = *(it++); 
    prev = map[prev];
    assert(prev != -1);

    if (it == text.end())
    {
        cerr << "Warning: sequence " << name << " was shorter than 2 bp!" << endl;
        text = "";
        return;
    }

    while (it != text.end())
    {
        int c = *(it++);
        c = map[c];
        assert(c != -1);
        
        output += code[prev][c];
        prev = c;
    }

    text = output;
}


void transform(string &text, string const &name, bool reverse)
{
    assert(!(reverse && rotation));    
    normalize(text, name);

    // Color transform first (deprecated)
    if (color)
        dna_to_color(text, name);

    // Appending a reverse complement
    string revcmpl(text);
    revstr(revcmpl);
    complement(revcmpl);
    text.append(1, '-');
    text.append(revcmpl);

    // Store everything reversed
    revstr(text);
}

void build(istream *input, string const &outputfile, unsigned samplerate, bool reverse, ulong estimatedLength, time_t wctime)
{
    TextCollectionBuilder *tcb = new TextCollectionBuilder(samplerate, estimatedLength);
    ulong j = 0;
    unsigned i = 0;
    string text = "";
    string name = "undef";
    string row;
    while (getline(*input, row).good()) 
    {
	if (row[0] == '>') 
        {
            row = row.substr(row.find_first_not_of(" \t", 1));
            row = row.substr(0, row.find_first_of(" \t"));
            i ++;
            j += text.size();

            // If no title given, generate simple id numbers
            if (row.empty())
            {
                stringstream ss;
                ss << i-2;
                row = ss.str();
            }
	    
            if (verbose && i % 1000000 == 0) 
            {
                cerr << "Inserting: " << row << " ("
                     << j/(1024*1024) << " MB, ";
//                if (estimatedLength)
//                    cerr << (100*j/estimatedLength) << "%, ";
                cerr << "elapsed " << std::difftime(time(NULL), wctime) << " s, " 
                     << std::difftime(time(NULL), wctime) / 3600 << " hours)" << endl;
            }

	    if (text.size() > 0)
            {
                assert(name != "undef");
                transform(text, name, reverse);
                if (reverse)
                    tcb->InsertText((uchar const *)text.c_str());
                else
                    tcb->InsertText((uchar const *)text.c_str(), name);
            }
            text.clear();
            name = row; // Cache the fasta title
	} 
        else 
            text.append(row);
    }
    // Flush the text buffer
    j += text.size();
    if (text.size() > 0)
    {
        transform(text, name, reverse);
        if (reverse)
            tcb->InsertText((uchar const *)text.c_str());
        else
            tcb->InsertText((uchar const *)text.c_str(), name);
    }
    text.clear();
    row.clear();

    std::cerr << "Warning: not thread-safe" << std::endl;

    if (verbose)
        std::cerr << "Creating new index with " << i << " sequences, total " << j << " bytes, " << j/1024 << " kb " 
                  << "(elapsed " << std::difftime(time(NULL), wctime) << " s, " 
                  << std::difftime(time(NULL), wctime) / 3600 << " hours)" << std::endl;

    // TODO currently never uses plain text
    bool storePlainText = false;
    if (reverse || rotation) 
        storePlainText = false;
    TextCollection* tc = tcb->InitTextCollection(storePlainText, color, patlen);
    delete tcb; tcb = 0;

    if (verbose) std::cerr << "Saving to file " << outputfile << endl 
                      << "(total wall-clock time " << std::difftime(time(NULL), wctime) << " s, " 
                      << std::difftime(time(NULL), wctime) / 3600 << " hours)" << endl;
    tc->save(outputfile);
    delete tc;
}

void print_usage(char const *name)
{
    cerr << "usage: " << name << " [options] <input> [output]" << endl
         << "Check README or `" << name << " --help' for more information." << endl;
}

void print_help(char const *name)
{
    cerr << "usage: " << name << " [options] <input> [output]" << endl << endl
         << "<input> is the input filename. "
         << "If no output filename is given, the index is stored as <input>.fmi" << endl
         << "or as <input>.rlcsa." << endl << endl
         << "Options:" << endl
         << " -s <int>, --sample-rate <int> Sampling rate for the index, a smaller number " << endl
         << "                               yields a bigger index but can decrease search " << endl
         << "                               time (default: " << TEXTCOLLECTION_DEFAULT_SAMPLERATE << ")." << endl
         << " -h, --help                    Display command line options." << endl
         << " -v, --verbose                 Print progress information." << endl;
}

int atoi_min(char const *value, int min, char const *parameter, char const *name)
{
    std::istringstream iss(value);
    int i;
    char c;
    if (!(iss >> i) || iss.get(c))
    {
        cerr << "readaligner: argument of " << parameter << " must be of type <int>, and greater than or equal to " << min << endl
             << "Check README or `" << name << " --help' for more information." << endl;
        std::exit(1);
    }

    if (i < min)
    {
        cerr << "readaligner: argument of " << parameter << " must be greater than or equal to " << min << endl
             << "Check README or `" << name << " --help' for more information." << endl;
        std::exit(1);
    }
    return i;
}


int main(int argc, char **argv) 
{
    std::cerr << "Warning: Reversing the string by default" << std::endl;

    /**
     * Parse command line parameters
     */
    if (argc == 1)
    {
        print_usage(argv[0]);
        return 1;        
    }
    unsigned samplerate = 0;
    bool reverse = false;

    static struct option long_options[] =
        {
            {"sample-rate", required_argument, 0, 's'},
            {"help",        no_argument,       0, 'h'},
            {"verbose",     no_argument,       0, 'v'},
            {0, 0, 0, 0}
        };
    int option_index = 0;
    int c;
    while ((c = getopt_long(argc, argv, "cR:s:F:hv",
                            long_options, &option_index)) != -1) 
    {
        switch(c) 
        {
        case 's':
            samplerate = atoi_min(optarg, 1, "-s, --sample-rate", argv[0]); 
            break;
        case 'h':
            print_help(argv[0]);
            return 0;
        case 'v':
            verbose = true; break;
        case '?':
            print_usage(argv[0]);
            return 1;
        default:
            print_usage(argv[0]);
            std::abort();
        }
    }
    
    cerr << "Warning: sampling is disabled!" << endl;

    if (samplerate && samplerate <= 3)
        cerr << "Warning: small samplerates (-s, --sample-rate) may yield infeasible index sizes" << endl;

    if (argc - optind < 1)
    {
        cerr << "readaligner: no input filename given!" << endl;
        print_usage(argv[0]);
        return 1;
    }
        
    if (argc - optind > 2)
        cerr << "Warning: too many filenames given! Ignoring all but first two." << endl;

    string inputfile = string(argv[optind++]);
    string outputfile = "";
    if (optind != argc)
        outputfile = string(argv[optind++]);
    
    istream *fp;
    if (inputfile == "-")
        fp = &std::cin;
    else
        fp = new std::ifstream(inputfile.c_str());

    if (!fp->good())
    {
        cerr << "builder: unable to read input file " << inputfile << endl;
        exit(1); 
    }

    cerr << std::fixed;
    cerr.precision(2);
    time_t wctime = time(NULL);

    // estimate the total input sequence length
//    fp->seekg(0, ios::end);
    long estLength = 1; //fp->tellg();
    if (estLength == -1)
    {
        cerr << "Warning: unable to estimate input file size" << endl;
        estLength = 0;
    }
//    fp->seekg(0);
    
    /**
     * Build forward/rotation index
     */
    if (verbose)
        cerr << "Building the forward index:" << endl;
    if (outputfile == "")
        outputfile = inputfile; // suffix will be added by TextCollection::save().
    if (rotation)
        outputfile += TextCollection::ROTATION_EXTENSION;
    build(fp, outputfile, samplerate, false, estLength, wctime);

    if (!reverse)
    {
        // No reverse index
        if (verbose) 
            std::cerr << "Skipping reverse indexing. Save complete. " 
                      << "(total wall-clock time " << std::difftime(time(NULL), wctime) << " s, " 
                      << std::difftime(time(NULL), wctime) / 3600 << " hours)" << endl;
        if (fp != &std::cin)
            delete fp;
        fp = 0;
        return 0; // all done
    }

    /**
     * Build reverse index
     */
    if (verbose)
        cerr << "Building the reverse index:" << endl;
    assert(!rotation);
    fp->clear(); // forget previous EOF
    fp->seekg(0);
    if (!fp->good())
    {
        cerr << "builder: unable to rewind the input file!" << endl;
        return 1;
    }

    outputfile += TextCollection::REVERSE_EXTENSION;
    build(fp, outputfile, samplerate, true, estLength, wctime);

    if (fp != &std::cin)
        delete fp;
    fp = 0;

    if (verbose) 
        std::cerr << "Save complete. "
                  << "(total wall-clock time " << std::difftime(time(NULL), wctime) << " s, " 
                  << std::difftime(time(NULL), wctime) / 3600 << " hours)" << endl;
    return 0;
}
 
