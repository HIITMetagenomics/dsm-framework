#include "TrieReader.h"

#include <unordered_set>
#include <vector>
#include <map>
#include <sstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <ctime>
#include <cstring>
#include <cstdlib> // exit()
#include <getopt.h>

using namespace std;

#define MAX_READERS 273
#define MAX_CHILDREN 4   // ACGT

typedef unordered_set<unsigned> readerset;

/**
 * Definitions for parsing command line options
 */
enum align_mode_t { mode_undef, mode_mismatch, mode_rotation,
                    mode_indel, mode_gap, mode_matepair };

enum parameter_t { long_opt_all = 256, long_opt_maxgap,
                   long_opt_minprefix, long_opt_skip, long_opt_nreads,
                   long_opt_debug, long_opt_recursion };

void print_usage(char const *name)
{
    cerr << "usage: " << name << " [options]" << endl
         << "Check README or `" << name << " --help' for more information." << endl;
}

void print_help(char const *name)
{
    cerr << "usage: " << name << " [options]" << endl
         << endl
         << "  <stdin>       A list of expected library names." << endl
         << endl 
         << "Options:" << endl
         << " --port <p>     Listen to port number p." << endl
         << " --topfreq <p>  Print the top-p output frequencies." << endl
         << " --toptimes <p> Print the top-p latencies." << endl
         << " --verbose      Print progress information." << endl
         << " --debug        More verbose but still safe." << endl
         << " --outputall    Even more verbose (not safe)." << endl;
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

bool mysort (TrieReader *i, TrieReader *j) 
{ 
    return (i->getRate() > j->getRate()); 
}

void printatr(vector<TrieReader *> tr)
{
    for (vector<TrieReader *>::iterator it = tr.begin(); it != tr.end(); ++it)
        cerr << " " << (*it)->getId();
    cerr << endl;
}

/**
 * Global data for recursion
 */
bool debug = false;
bool outputall = false; // For debugging only!
bool verbose = false;
int toptimes = 0;
int topfreq = 0;
unsigned pmin = 2;
time_t wctime = time(NULL);
ulong total_paths = 0;
ulong total_output = 0;
ulong total_occs = 0;
vector<TrieReader *> allreaders;
vector<ulong> freqhistogram;
int dnatoi[256];
char itodna[MAX_CHILDREN];
string path;

inline bool moreChildren(vector<readerset> const &children)
{
    for (size_t i = 0; i < MAX_CHILDREN; ++i)
        if (children[i].size())
            return true;
    return false;
}


/**
 * Updates the currently active triereaders by reading their next child.
 * Returns true if there are children remaining to be processed.
 */
bool readChildren(readerset const &atr, vector<readerset> &children)
{
    for (readerset::const_iterator it = atr.begin(); it != atr.end(); ++it)
    {
        TrieReader *tr = allreaders[*it];
        if (tr->hasChild())
        {
            int c = tr->readChild();
            if (dnatoi[c] == -1)
            {
                cerr << "readChildren(): received invalid readChild byte " << c << endl;
                exit(1);
            }
            children[dnatoi[c]].insert(*it);
        }
    }
    return moreChildren(children);
}

int readChild(unsigned reader)
{
    TrieReader *tr = allreaders[reader];
    if (tr->hasChild())
    {
        int c = tr->readChild();
        if (dnatoi[c] == -1)
        {
            cerr << "readChildren(): received invalid readChild byte " << c << endl;
            exit(1);
        }
        return c;
    }
    return 0;
}

/**
 * Traversing when there is exactly one active reader
 * Here we assume pmin > 1, and do not output.
 */
void traverseOne(unsigned reader)
{
    // traverse children
    while (readChild(reader) != 0)
        traverseOne(reader);

    // Note: Even if output == false, we need to iterate through readOccs()
    TrieReader *tr = allreaders[reader];
    tr->readOccs();
    tr->readClose();  // Closing parenthesis is read & checked
    ++total_paths; // No output, pmin > 1 here
}


/**
 * Traversing when there is exactly one active reader
 * Here we assume pmin == 1, and output.
 */
void traverseOneWithOutput(unsigned reader)
{
    unsigned children = 0;
    int c = 0;
    // traverse children
    while ((c = readChild(reader)) != 0)
    {
        ++children;
        path.push_back(c);
        traverseOneWithOutput(reader);
        path.resize (path.size () - 1); // path.pop_back();
    }

    // No output for empty path
    if (path.empty()) return;

    // Note: Even if output == false, we need to iterate through readOccs()
    TrieReader *tr = allreaders[reader];
    ulong occs = tr->readOccs();

    if (path.size() <= 6)
        tr->checkR(); // Checksum is written for nodes at levels <7.

    tr->readClose();  // Closing parenthesis is read & checked

    // Always output here...
    ++total_paths;
    ++total_output;
    ++freqhistogram[1];
    cout << path << " " << tr->getId() << ":" << occs << '\n';
    ++total_occs;    
}

void traverse(readerset const &treaders)
{
    if (outputall || (path.size() <= (5+ 2*(unsigned)debug) && verbose))
    {
        // Sort the readers by least activity (longest pause time)
        if (toptimes)
        {
            vector<TrieReader *> sortedtrs = allreaders; // Copy
            sort(sortedtrs.begin(), sortedtrs.end(), mysort);
            cerr << "[";
            for (vector<TrieReader *>::iterator it = sortedtrs.begin(); it != sortedtrs.end(); ++it)
            {
                if (distance(sortedtrs.begin(), it) >= toptimes)
                    break; // Print only the top highest times
                if (distance(sortedtrs.begin(), it) > 10 && ((int)(*it)->getRate() == 0))
                {
                    cerr << "...";
                    break; // Exclude those that have refreshed less than second ago.
                }
                cerr << ' ' << (*it)->getId() << '/' << (int) (*it)->getRate() << "ys";
            }
            cerr << ']' << endl;
        }

        if (topfreq)
        {
            cerr << "<";
            for (vector<ulong>::iterator it = freqhistogram.begin(); it != freqhistogram.end(); ++it)
            {
                if (distance(freqhistogram.begin(), it) < topfreq 
                    || distance(it, freqhistogram.end()) <= topfreq)
                    cerr << " " << *it;
                else if (distance(freqhistogram.begin(), it) == topfreq)
                    cerr << " ..."; 
            }
            cerr << ">" << endl;
        }
        cerr << "current path is " << path
             << " (" << treaders.size() << " active, " 
             << total_output << " reported, " 
             << total_occs << " occs, " 
             << std::difftime(time(NULL), wctime) << " s, " 
             << std::difftime(time(NULL), wctime) / 3600 << " hrs)" << endl;
    }

    if (treaders.size() == 1)
    {
        readerset::const_iterator it = treaders.begin();
        if (pmin > 1)
            traverseOne(*it);
        else
            traverseOneWithOutput(*it);
        return;
    }

    // Iterate and collect children
    readerset atr = treaders;
    vector<readerset> children(MAX_CHILDREN);
    unsigned numberOfChildren = 0;
    while (readChildren(atr, children))
    {
        // Process the lexicographically smallest child:
        int i = 0;
        while (children[i].size() == 0)
            ++i;

        atr = children[i]; // Next round needs to update only these readers
        ++numberOfChildren;

        path.push_back(itodna[i]);
        traverse(children[i]);
        path.resize (path.size () - 1);
        children[i].clear(); // Close the subtree.
    } 

    // No output for empty path
    if (path.empty()) return;

    // Sanity check
    if (treaders.size() > allreaders.size())
    {
        cerr << "error: trearders size was greater than expected: " << treaders.size() << " > " << allreaders.size() << endl;
        exit(1);
    }
    
    // Output currently active readers (post-order)
    // Note: Output only if path is not in every set
    // Note: Even if output == false, we need to iterate through readOccs()
    for (readerset::const_iterator it = treaders.begin(); it != treaders.end(); ++it)
    {
        TrieReader *tr = allreaders[*it];
        tr->readOccs();
        if (path.size() <= 6)
            tr->checkR(); // Checksum is written for nodes at levels <7.
        tr->readClose();  // Closing parenthesis is read & checked
    }
    bool output = true;
    if (treaders.size() == allreaders.size())
        output = false;
    if (treaders.size() < pmin)
        output = false;
    if (numberOfChildren == 1 && treaders.size() == atr.size())
        output = false; // not left branching
    
    ++total_paths;
    if (output) 
    {
        ++total_output;
        ++freqhistogram[treaders.size()];
        cout << path;
        
        for (readerset::const_iterator it = treaders.begin(); it != treaders.end(); ++it)
        {
            TrieReader *tr = allreaders[*it];
            cout << ' ' << tr->getId() << ':' << tr->getOccs();
            ++total_occs;
        }
        cout << '\n';
    }
}

int main(int argc, char **argv) 
{
    /**
     * Init mapping
     */
    for (int i = 0; i < 256; ++i)
        dnatoi[i] = -1;
    dnatoi['A'] = 0;  itodna[0] = 'A';
    dnatoi['C'] = 1;  itodna[1] = 'C';
    dnatoi['G'] = 2;  itodna[2] = 'G';
    dnatoi['T'] = 3;  itodna[3] = 'T';
    //dnatoi['N'] = 4;
    if (MAX_CHILDREN != 4)
    {
        cerr << "main(): MAX_CHILDREN was different" << endl;
        return 1;
    }

    /**
     * Parse command line parameters
     */
    int portno = 54666;

    static struct option long_options[] =
        {
            {"pmin",   required_argument, 0, 'P'},
            {"port",   required_argument, 0, 'p'},
            {"topfreq",   required_argument, 0, 'F'},
            {"toptimes",  required_argument, 0, 'T'},
            {"verbose",   no_argument,       0, 'v'},
            {"help",      no_argument,       0, 'h'},
            {"debug",     no_argument,       0, long_opt_debug},
            {"outputall", no_argument,       0, 'A'},
            {0, 0, 0, 0}
        };
    int option_index = 0;
    int c;
    while ((c = getopt_long(argc, argv, "P:p:F:T:vhA",
                                 long_options, &option_index)) != -1) 
    {
        switch(c) 
        {
        case 'P':
            pmin = atoi_min(optarg, 1, "-P, --pmin", argv[0]) ; break;
        case 'p':
            portno = atoi_min(optarg, 1024, "-p, --port", argv[0]) ; break;
        case 'F':
            topfreq = atoi_min(optarg, 1, "--recursion", argv[0]) ; break;
        case 'T':
            toptimes = atoi_min(optarg, 1, "--recursion", argv[0]) ; break;
        case 'v':
            verbose = true;
            break;
        case long_opt_debug:
            debug = true; break;
        case 'A':
            outputall = true; break;
        case '?': 
        case 'h':
            print_help(argv[0]);
            return 1;
        default:
            print_usage(argv[0]);
            std::abort ();
        }
    }

    /**
     * Read list of input files
     */
    string line;
    map<string,int> libtoid;
    cerr << "Expected inputs:";
    while (cin.good())
    {
        getline(cin,line);
        if (line.empty())
        {
            if (debug) cerr << endl << "skipping empty line at " << allreaders.size() << "..." << endl;
            continue;
        }
        int id = libtoid.size();
        if (verbose)
            cerr << " " << id << " : " << line;

        if (libtoid.find(line) != libtoid.end())
        {
            cerr << endl << "DUPLICATE CLIENT NAME IN stdin! id = " << id << ", name = " << line << endl;
            return 1;
        }

        libtoid[line] = id;
    }
    cerr << endl;

    if (verbose)
        cerr << "Reading " << libtoid.size() << " input pipes from port " << portno << endl;

    if ( libtoid.size() > MAX_READERS )
    {
        cerr << "Too many input readers requested! MAX_READERS was " << MAX_READERS << endl;
        return 1;
    }

    /**
     * Init socket
     */
    int sockfd = ServerSocket::init(portno);

    /**
     * Construct input readers
     */
    allreaders = vector<TrieReader *>(libtoid.size(), 0);
    while (libtoid.size())
    {
        // Listen for incoming connections...
        ServerSocket *ss = ServerSocket::create(sockfd);
        char c = ss->getc();
        if (c != 'S')
        {
            cerr << "received invalid start byte: " << (int)c << endl;
            return 1;
        }
        string name = ss->getstring();
        map<string,int>::iterator found = libtoid.find(name);
        if (found == libtoid.end())
        {
            cerr << "received invalid libname: \"" << name << "\"" << endl;
            return 1;
        }
        cerr << "new connection id = " << found->second << ", name = " << found->first << " (" << libtoid.size() << " pending)" << endl;

        TrieReader *tr = new TrieReader(found->second, found->first, ss, verbose, debug);
        if (! tr->good())
        {
            cerr << "unable to open input file: " << line << endl;
            return 1;
        }
        if (allreaders[found->second])
        {
            cerr << "DUPLICATE CONNECTING CLIENT! id = " << found->second << ", name = " << name << endl;
            return 1;
        }
        allreaders[found->second] = tr;
        libtoid.erase(found);
    }

    freqhistogram = vector<ulong>(allreaders.size(), 0); 
    wctime = time(NULL);
    path.reserve(1024*1024);

    readerset rb;
    for (size_t i = 0; i < allreaders.size(); ++i)
        rb.insert(i); // Set all first allreaders.size() bits to 1
    traverse(rb);

    for (vector<TrieReader *>::iterator it = allreaders.begin(); it != allreaders.end(); ++it)
    {
        if (distance(allreaders.begin(), it) != (*it)->getId())
            cerr << "Warning: ID was changed for " << (*it)->getId() << " vs " << distance(allreaders.begin(), it) << endl;
        (*it)->checkEof();
        delete *it;
    }

    if (verbose)
    {	
        cerr << "Number of paths: " << total_paths << endl
             << "Number of reported paths: " << total_output << endl
             << "Number of reported occs: " << total_occs << endl;
        cerr << "Wall-clock time: " << std::difftime(time(NULL), wctime) << " seconds (" 
             << std::difftime(time(NULL), wctime) / 3600 << " hours)" << endl;
    }
}
