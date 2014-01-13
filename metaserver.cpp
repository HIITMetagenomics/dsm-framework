#include "TrieReader.h"

#include <unordered_set>
#include <utility>
#include <vector>
#include <map>
#include <sstream>
#include <iostream>
#include <cmath>
#include <string>
#include <algorithm>
#include <ctime>
#include <cstring>
#include <cstdlib> // exit()
#include <getopt.h>

using namespace std;

#define MAX_READERS 273
#define MAX_CHILDREN 4   // ACGT
#define PTHRESHOLD 10.0

typedef unordered_set<unsigned> readerset;

/**
 * Definitions for parsing command line options
 */
enum parameter_t { long_opt_debug = 256, long_opt_discriminative, long_opt_pmax };

void print_usage(char const *name)
{
    cerr << "usage: " << name << " [options]" << endl
         << "Check README or `" << name << " --help' for more information ." << endl;
}

void print_help(char const *name)
{
    cerr << "usage: " << name << " [options] < names.txt" << endl
         << endl
         << "  names.txt         A list of expected library names (read from stdin)." << endl
         << endl 
         << "Mandatory option:" << endl
         << " -E,--emax <double> Maximum entropy to output." << endl  << endl
      //         << "--discriminative <int>  Discriminative mining. " << endl << endl
         << "Other options:" << endl
         << " -p,--port <p>      Listen to port number p." << endl
         << " -P,--pmin <int>    p_min value (min. number of samples to have occ's in)," << endl
         << "                    default 2." << endl
         << " --pmax <int>       p_max value (max. number of samples to have occ's in)," << endl
         << "                    default no-limit. Set p_min=p_max=1 to restrict the" << endl
         << "                    output to sample-specific substrings." << endl
         << " -e,--emin <double> Minimum entropy to output (default 0.0)" << endl
         << " -F,--topfreq <p>   Print the top-p output frequencies." << endl
         << " -T,--toptimes <p>  Print the top-p latencies." << endl
         << " -v,--verbose       Print progress information." << endl
         << " --debug            More verbose but still safe." << endl
         << " -A,--outputall     Even more verbose (not safe)." << endl;
}

int atoi_min(char const *value, int min, char const *parameter, char const *name)
{
    std::istringstream iss(value);
    int i;
    char c;
    if (!(iss >> i) || iss.get(c))
    {
        cerr << name << ": argument of " << parameter << " must be of type <int>, and greater than or equal to " << min << endl
             << "Check README or `" << name << " --help' for more information." << endl;
        std::exit(1);
    }

    if (i < min)
    {
        cerr << name << ": argument of " << parameter << " must be greater than or equal to " << min << endl
             << "Check README or `" << name << " --help' for more information." << endl;
        std::exit(1);
    }
    return i;
}

double atof_min(char const *value, double min, char const *parameter, char const *name)
{
    std::istringstream iss(value);
    double i;
    char c;
    if (!(iss >> i) || iss.get(c))
    {
        cerr << name << ": argument of " << parameter << " must be of type <double>, and greater than or equal to " << min << endl
             << "Check README or `" << name << " --help' for more information." << endl;
        std::exit(1);
    }

    if (i < min)
    {
        cerr << name << ": argument of " << parameter << " must be greater than or equal to " << min << endl
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
int positivesets = -1;
int toptimes = 0;
int topfreq = 0;
unsigned mindepth = 0;

bool discriminative = false;
int fisheralt = 0;
unsigned pmin = 2;
unsigned pmax = 0;
double emin = 0.0;
double emax = -1.0;
double smallest_entropy = 1000.0;
double largest_entropy = -1000.0;

time_t wctime = time(NULL);
ulong tnbin_discard = 0;
ulong total_paths = 0;
ulong total_output = 0;
ulong total_occs = 0;
vector<TrieReader *> allreaders;
vector<ulong> freqhistogram;
int dnatoi[256];
char itodna[MAX_CHILDREN];
string path;
double *posFreqVector = 0, *negFreqVector = 0;
//unsigned haltSent = 0;

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
/*        if (debug)
        {
            cerr << "child table: ";
            for (map<char,vector<TrieReader *> >::iterator it  = children.begin(); it != children.end(); ++it)
            {
                cerr << it->first << "[";
                for (vector<TrieReader *>::iterator jt = it->second.begin(); jt != it->second.end(); ++jt)
                    cerr << " " << (*jt)->getId();
                cerr << "]";
            }
            cerr << endl;
            }*/

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

/* FIXME this should not occur... if (path.size() <= 6)
   tr->checkR(); // Checksum is written for nodes at levels <7.*/

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

    char leftChar = tr->readClose();  // Closing parenthesis is read & checked

    // Check if we output?
    ++total_paths;
    if ((leftChar == '0' || leftChar == 'N') && path.size() >= mindepth) // assert (pmin == 1)
    {
        ++total_output;
        ++freqhistogram[0];
        cout << path << " " << tr->getId() << ":" << occs << '\n';
        ++total_occs;
    }
}

void traverse(readerset const &treaders)
{
    if (outputall || (path.size() <= (5 + 2*(unsigned)debug) && verbose))
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
//        for (vector<char>::const_iterator pathit = path.begin(); pathit != path.end(); ++pathit)
//            cerr << *pathit;
         << " (" << treaders.size() << " active, " << total_output << " reported, " << total_occs << " occs, " << std::difftime(time(NULL), wctime) << " s, " 
             << std::difftime(time(NULL), wctime) / 3600 << " hrs), entropies [" << smallest_entropy << ", " << largest_entropy << "], tnbin_discard = " << tnbin_discard << endl;
    }

    if (treaders.size() == 1 && pmin > 1)
    {
        readerset::const_iterator it = treaders.begin();
        traverseOne(*it);
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
    
    // Traverse currently active readers (post-order)
    // Note: Output only if path is not in every set
    // Note: Even if output == false, we need to iterate through readOccs()
    char leftChar = 0;

    int pfv = 0, nfv = 0;            // used to compute discriminative mining
    ulong sumN = allreaders.size();  // used to compute \sum_j n_j + d*1
    double sumNlogN = 0;             // used to compute \sum_i (n_i+1) log (n_i+1)
    if (discriminative)
    {
        for (int j = 0; j < positivesets; ++j)
            posFreqVector[j] = 0;
        for (unsigned j = 0; j < allreaders.size() - positivesets; ++j)
            negFreqVector[j] = 0;
    }
    for (readerset::const_iterator it = treaders.begin(); it != treaders.end(); ++it)
    {
        TrieReader *tr = allreaders[*it];
        ulong freq = tr->readOccs();
        if (discriminative)
        {
            if (tr->isPositive())
                posFreqVector[pfv++] = freq;
            else
                negFreqVector[nfv++] = freq;
        }
        // Update entropy
        sumN += freq;
        sumNlogN += (double)(freq+1) * log(freq+1)/log(2);

        if (path.size() <= 6)
            tr->checkR(); // Checksum is written for nodes at levels <7.
        char lChar = tr->readClose();  // Closing parenthesis is read & checked
        if (leftChar == 0)
            leftChar = lChar;
        else if (leftChar != lChar)
            leftChar = 'N';
    }
    double entropy = log(sumN)/log(2) - sumNlogN/(double)sumN;
    if (smallest_entropy > entropy)
        smallest_entropy = entropy;
    if (largest_entropy < entropy)
        largest_entropy = entropy;

    assert(leftChar != 0);
    assert(!discriminative || pfv <= positivesets);
    assert(!discriminative || nfv <= allreaders.size() - positivesets);
    if (discriminative && pfv == 0)
        posFreqVector[0] = 1;  // All zero values do not work with TNBin
    if (discriminative && nfv == 0)
        negFreqVector[0] = 1;  // All zero values do not work with TNBin

    /**
     * Test for output conditions
     */
    bool output = true;
    if (path.size() < mindepth)
        output = false;
    if (pmax != 0 && treaders.size() > pmax)
        output = false;
    if (treaders.size() < pmin)
        output = false;
    if (emax > 0 && (entropy < emin || entropy > emax))
        output = false;

    if (numberOfChildren == 1 && treaders.size() == atr.size())
        output = false; // not right branching
    if (leftChar == 'A' || leftChar == 'C' || leftChar == 'G' || leftChar == 'T')
        output = false; // not left branching

    double pvalue = 0;
    // Output condition for discriminative mining
    if (output && discriminative)
    {
        /** 
         * Using Fisher test with
         * a = # of positive samples with (>x occurrences of) string
         * b = # of positive samples without (>x occurrences of) string
         * c = # of negative samples with (>x occurrences of) string
         * d = # of negative samples without (>x occurrences of) string 
         *
        int a = 0;
        for (readerset::const_iterator it = treaders.begin(); it != treaders.end(); ++it)
            if (allreaders[*it]->isPositive())
                ++a;
        int b = positivesets - a;
        int c = treaders.size() - a;
        int d = allreaders.size() - positivesets - c;

        pvalue = fisher_test(a, b, c, d, fisheralt);
        ** End of Fisher test
        **/

        /**
         * Using TNBin3 test
         *
         * Assert: posFreqVector and negFreqVector are initialized
         **
        pvalue = tnbin_test(positivesets, posFreqVector, allreaders.size() - positivesets, negFreqVector);
        if (pvalue < PTHRESHOLD)
        {
            tnbin_discard ++;
            output = false;
	    }*/

        // DEBUG OUTPUT
        /*
        for (int j = 0; j < positivesets; ++j)
            printf("%.0f,", posFreqVector[j]);
        printf(" ");
        for (unsigned j = 0; j < allreaders.size() - positivesets; ++j)
            printf("%.0f,", negFreqVector[j]);
        printf("\n");
        output = false;*/
    }
    
    ++total_paths;
    if (output) 
    {
        ++total_output;
        ++freqhistogram[treaders.size() - 1];
        printf("%s", path.c_str());
        if (discriminative)
            printf(" %f", pvalue);
        else
            printf(" %f", entropy);
        
        for (readerset::const_iterator it = treaders.begin(); it != treaders.end(); ++it)
        {
            TrieReader *tr = allreaders[*it];            
            printf(" %d:%lu", tr->getId(), tr->getOccs());
            ++total_occs;
        }
        printf("\n");
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
    if (argc <= 1)
    {
        print_usage(argv[0]);
        return 1;
    }

    int portno = 54666;

    static struct option long_options[] =
        {
            {"discriminative", required_argument, 0, long_opt_discriminative},
            {"pmin",           required_argument, 0, 'P'},
            {"pmax",           required_argument, 0, long_opt_pmax},
            {"port",           required_argument, 0, 'p'},
            {"mindepth",       required_argument, 0, 'm'},
            {"emin",           required_argument, 0, 'e'},
            {"emax",           required_argument, 0, 'E'},
            {"topfreq",        required_argument, 0, 'F'},
            {"toptimes",       required_argument, 0, 'T'},
            {"verbose",        no_argument,       0, 'v'},
            {"help",           no_argument,       0, 'h'},
            {"debug",          no_argument,       0, long_opt_debug},
            {"outputall",      no_argument,       0, 'A'},
            {0, 0, 0, 0}
        };
    int option_index = 0;
    int c;
    while ((c = getopt_long(argc, argv, "P:p:m:e:E:F:T:vhA",
                                 long_options, &option_index)) != -1) 
    {
        switch(c) 
        {
        case long_opt_discriminative:
	  cerr << "error: option --discriminative is not supported!" << endl;
	  return 1;
            discriminative = true; 
            fisheralt = atoi_min(optarg, -1, "--discriminative", argv[0]);
            break;
        case 'P':
            pmin = atoi_min(optarg, 1, "-P, --pmin", argv[0]) ; break;
        case long_opt_pmax:
            pmax = atoi_min(optarg, 1, "--pmax", argv[0]) ; break;
        case 'p':
            portno = atoi_min(optarg, 1024, "-p, --port", argv[0]) ; break;
        case 'm':
            mindepth = atoi_min(optarg, 1, "-m, --mindepth", argv[0]);
            cerr << "using min depth = " << mindepth << endl; 
            break;
        case 'e':
            emin = atof_min(optarg, 0, "-e, --emin", argv[0]) ; break;
        case 'E':
            emax = atof_min(optarg, 0, "-E, --emax", argv[0]) ; break;
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

    if ((!discriminative && emax < 0) || (discriminative && emax > 0))
    {
        cerr << argv[0] << ": error: expecting parameter --emax" << endl;
        return 1;
    }


    if (emin > emax && !discriminative)
    {
        cerr << argv[0] << ": error: -e <double> must be smaller than or equal to -E <double>" << endl;
        return 1;
    }

    // Parse filenames
/*    if (argc - optind != 1)
    {
        cerr << argv[0] << ": expecting one filename" << endl;
        print_usage(argv[0]);
        return 1;
        }*/

    /**
     * Read list of input files
     */
    positivesets = 0; // Number of 'positive' sets
    string line;
    map<string,pair<int,bool> > libtoid;
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
        string name = line.substr(0, line.find_first_of('\t'));
        bool positive = false;
        if (discriminative)
        {
            // Split the line at tab
            string::size_type pos = line.find_first_of('\t');
            if (pos == string::npos 
                || (line[pos+1] != '0' && line[pos+1] != '1'))
            {
                cerr << endl << "Invalid input file for discriminative mining: expecting <run> <boolean> pairs as input" << endl;
                return 1;
            }
            name = line.substr(0, pos);
            if (line[pos+1] == '1')
            {
                positive = true;
                positivesets ++;
            }
        }
        if (verbose && id < 100)
            cerr << " " << id << " : " << name;
        if (verbose && discriminative && id < 100)
            cerr << " (" << positive << ")";
        if (verbose && id == 100)
            cerr << " ...";

        if (libtoid.find(name) != libtoid.end())
        {
            cerr << endl << "DUPLICATE CLIENT NAME IN stdin! id = " << id << ", name = " << name << endl;
            return 1;
        }

        libtoid[name] = make_pair(id,positive);
    }
    cerr << endl;

    if (verbose)
    {
        cerr << "Reading " << libtoid.size() << " input pipes from port " << portno << endl;
        if (discriminative)
            cerr << "Expecting " << positivesets << " positive feeds." << endl
                 << " using pthreshold = " << PTHRESHOLD << " and alternative = " << fisheralt << endl;
        else
            cerr << "Using emin = " << emin << " and emax = " << emax << endl;
    }
        
    posFreqVector = 0;
    negFreqVector = 0;
    if (discriminative)
    {
        posFreqVector = new double[positivesets];
        negFreqVector = new double[libtoid.size() - positivesets];
    }

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
        map<string,pair<int,bool> >::iterator found = libtoid.find(name);
        if (found == libtoid.end())
        {
            cerr << "received invalid libname: \"" << name << "\"" << endl;
            return 1;
        }
        pair<int,bool> value = found->second;
        int id = value.first;
        bool positive = value.second;

        cerr << "new connection id = " << id << ", name = " << found->first << " (" << positive <<  ", " << libtoid.size()-1 << " pending";
        if (libtoid.size() < 10)
            for (map<string,pair<int,bool> >::const_iterator it = libtoid.begin(); it != libtoid.end(); ++it)
                cerr << ", " << it->first; 
        cerr << ")" << endl;

        TrieReader *tr = new TrieReader(id, found->first, ss, verbose, debug, positive);
        if (! tr->good())
        {
            cerr << "unable to open input file: " << line << endl;
            return 1;
        }
        if (allreaders[id])
        {
            cerr << "DUPLICATE CONNECTING CLIENT! id = " << id << ", name = " << name << endl;
            return 1;
        }
        allreaders[id] = tr;
        libtoid.erase(found);
    }

    freqhistogram = vector<ulong>(allreaders.size(), 0); 
    wctime = time(NULL);
    path.reserve(1024*1024);



    readerset rb;
    for (size_t i = 0; i < allreaders.size(); ++i)
        rb.insert(i); // Set all first allreaders.size() bits to 1
    traverse(rb);

    /**
     * Init tree
     */
    /*    TrieNode *root = new TrieNode(allreaders);
    root->updateReaders();
    TrieNode *cur = root->firstChild();*/

    // Loop while the root node has children
/*    while (!cur->isRoot() || cur->hasChildren())
    {  
        if (debug || (cur->getLevel() <= 5 && verbose))
        {
            cerr << "Current node has path: " << cur->path() << endl;
        }

        // iterate and update each active reader at this node
        vector<TrieReader *> areaders = cur->getActiveReaders();
        for (vector<TrieReader *>::iterator it = allreaders.begin(); it != allreaders.end(); ++it)
        {
            TrieReader *tr = *it;
            if (tr->hasChild())
            {
                char c = tr->readChild();
                if (debug) cerr << "moving reader to child " << c << endl; 
                (cur->getChild(c))->addReader(tr);
                cur->removeReader(tr);
            }
        }

        if (cur->hasChildren())
        {
            // Traverse all the children first
            cur = cur->firstChild();
        }
        else
        {
            // No children, we output results in post-order and close the node
            if (debug)
                cerr << "outputting results for node at path: " << cur->path() << endl;
            
            vector<TrieReader *> areaders = cur->getActiveReaders();

            char c = cur->getSymbol();
            cur = cur->getParent();
            cur->removeChild(c);
        }
    }

    if (root->size() != 1)
        cerr << "WARNING:  Tree size was " << root->size() << " at exit." << endl;

    // clean up
    delete root; root = 0;*/
    for (vector<TrieReader *>::iterator it = allreaders.begin(); it != allreaders.end(); ++it)
    {
        if (distance(allreaders.begin(), it) != (*it)->getId())
            cerr << "Warning: ID was changed for " << (*it)->getId() << " vs " << distance(allreaders.begin(), it) << endl;
        (*it)->checkEof();
        delete *it;
    }

    delete [] posFreqVector;
    delete [] negFreqVector;

    if (verbose)
    {	
        cerr << "Number of paths: " << total_paths << endl
             << "Number of reported paths: " << total_output << endl
             << "Number of reported occs: " << total_occs << endl
             << "Number of TNBin discarded paths: " << tnbin_discard << endl
             << "Smallest and largest entropies encountered: " <<  smallest_entropy << " and " << largest_entropy << endl;        
        cerr << "Wall-clock time: " << std::difftime(time(NULL), wctime) << " seconds (" 
             << std::difftime(time(NULL), wctime) / 3600 << " hours)" << endl;
    }
}
