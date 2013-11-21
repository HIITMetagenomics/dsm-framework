#include "Query.h"
#include "OutputWriter.h"
#include "EnumerateQuery.h"

#include <sstream>
#include <iostream>
using std::cout;
using std::endl;
using std::cerr;
using std::cin;
#include <string>
using std::string;
#include <vector>
using std::vector;
#include <ctime>
#include <cstring>
#include <getopt.h>
#include <sys/resource.h>
#include <omp.h>

struct host_info {
    string name;
    int port;
    string enforcepath;
};

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
    cerr << "usage: " << name << " [options] <index>  < hostinfo.txt" << endl
         << "Check README or `" << name << " --help' for more information." << endl;
}

void print_help(char const *name)
{
    cerr << "usage: " << name << " [options] <index>  < hostinfo.txt" << endl
         << endl
         << " <index>        Index file." <<endl
         << " hostinfo       Text file containing list of hosts to connect to," << endl
         << "                port n:o, and path to enumerate." <<endl <<endl
         << "Options: " <<endl
         << " --check        Check integrity of index and quit." << endl
         << " --port <p>     Connect to port <p>." <<endl
         << " --verbose      Print progress information." << endl
         << "Debug options:"<<endl
         << " --debug        Print more progress information." << endl;
}

int atoi_min(char const *value, int min, char const *parameter, char const *name)
{
    std::istringstream iss(value);
    int i;
    char c;
    if (!(iss >> i) || iss.get(c))
    {
        cerr << "metaenumerate: argument of " << parameter << " must be of type <int>, and greater than or equal to " << min << endl
             << "Check README or `" << name << " --help' for more information." << endl;
        std::exit(1);
    }

    if (i < min)
    {
        cerr << "metaenumerate: argument of " << parameter << " must be greater than or equal to " << min << endl
             << "Check README or `" << name << " --help' for more information." << endl;
        std::exit(1);
    }
    return i;
}

string libname(string const &full)
{
    size_t found;
    found=full.find_last_of("/\\");
    if (found == string::npos)
        found = -1;
    string tmp = full.substr(found+1);
    found=tmp.find_first_of(".");
    return tmp.substr(0, found);
}

/**
 * Simple check to compare index size vs backward search
 */
int checkIndex(TextCollection *tc, bool verbose)
{
    ulong n = tc->getLength();
    ulong totals = 0;

    std::cerr.imbue(std::locale("en_US.UTF-8"));
    if (verbose) cerr << endl;

    // ALPHABET has been defined in BTSearch
//    for (const char *c = Query::ALPHABET_DNA; c < Query::ALPHABET_DNA + Query::ALPHABET_SIZE; ++c) 
    for (uchar c = 0; c < 255; ++c)
    {
        ulong nmin = tc->LF(c, 0lu - 1);
        ulong nmax = tc->LF(c, n-1)-1;    
        if (nmax >= nmin)
        {
            if (verbose) cerr << (int)c << ": " << nmax-nmin+1 << endl;
            totals += nmax - nmin + 1;
        }
/*        else
        {
            cout << "FAILED *********** ";
            cerr << "FAILED *********** ";
            exit(1);
            }*/
    }

    if (n == totals)
        cerr << "OK     ";
    else
        cerr << "FAILED *********** ";

    cerr  << std::showbase << "n = " << n << ", total = " << totals << endl;
    return 0;
}


int main(int argc, char **argv) 
{
    /**
     * Parse command line parameters
     */
    if (argc <= 1)
    {
        print_usage(argv[0]);
        return 0;
    }

    unsigned fmin = 10;
    unsigned maxdepth = ~0u;
    bool checkonly = false;
    bool debug = false;
    bool verbose = false;

#ifndef PARALLEL_SUPPORT
            cerr << "metaenumerate: Parallel processing not currently available!" << endl 
                 << "Please recompile with parallel support; see README for more information." << endl;
            return 1;
#endif

    static struct option long_options[] =
        {
            {"fmin",      required_argument, 0, 'f'},
            {"maxdepth",  required_argument, 0, 'M'},
            {"check",     no_argument,       0, 'C'},
            {"verbose",   no_argument,       0, 'v'},
            {"debug",     no_argument,       0, long_opt_debug},
            {0, 0, 0, 0}
        };
    int option_index = 0;
    int c;
    while ((c = getopt_long(argc, argv, "f:M:Cvh",
                                 long_options, &option_index)) != -1) 
    {
        switch(c) 
        {
        case 'f':
            fmin = atoi_min(optarg, 1, "-f, --fmin", argv[0]); break;
        case 'M':
            maxdepth = atoi_min(optarg, 1, "-M, --maxdepth", argv[0]); break;
        case 'C':
            checkonly = true;
            break;
        case 'v':
            verbose = true;
            break;
        case long_opt_debug:
            debug = true; break;
        case '?': 
        case 'h':
            print_usage(argv[0]);
            return 1;
        default:
            print_usage(argv[0]);
            std::abort ();
        }
    }

    // Parse filenames
    if (argc - optind != 1)
    {
        cerr << argv[0] << ": expecting index filename" << endl;
        print_usage(argv[0]);
        return 1;
    }
    string indexfile = string(argv[optind++]);

    // Parse host name, port number and enforced path
    if (verbose) cerr << "reading stdin for hostinfo.txt" << endl;
    vector<struct host_info> hosts;
    while (cin.good())
    {
        struct host_info hi;
        cin >> hi.name;
        if (hi.name.empty())
            break;
        cin >> hi.port;
        if (hi.port < 1024)
        {
            cerr << "error: invalid port number: " << hi.port << endl;
            abort();
        }
        cin >> hi.enforcepath;
        if (hi.enforcepath.empty())
        {
            cerr << "error: invalid enforced path: " << hi.enforcepath << endl;
            abort();
        }
        if (verbose)
            cerr << "host_info " << hosts.size() << ": \"" << hi.name << "\" : " << hi.port << ", \"" << hi.enforcepath << "\"" << endl;
        hosts.push_back(hi);
    }

    if (!hosts.size())
    {
        cerr << "error: empty host info" << endl;
        abort();
    }

    /**
     * Initialize shared data structures
     */
    if (verbose) cerr << "Loading index " << indexfile << endl;
    TextCollection *tc = 0; 
    tc = TextCollection::load(indexfile);
    // Sanity checks
    if (!tc) {
        cerr << argv[0] << ": could not read index file " << indexfile << endl;
        return 1;
    }
    if (tc->isColorCoded())
    {
        cerr << argv[0] << ": index " << indexfile << " cannot be color coded." << endl;
        return 1;
    }

    /**
     * Simple check to compare index size vs backward search
     */
    if (checkonly) 
    {
        cerr << indexfile << ": ";
//        fflush(stdout);
        return checkIndex(tc, verbose);
    }

    // Dummy output writer
    OutputWriter *outputw = OutputWriter::build(OutputWriter::output_tabs, "");
    
    // Shared counters
    unsigned total_found = 0;
    ulong total_occs = 0;
    time_t wctime = time(NULL);

#pragma omp parallel num_threads(hosts.size())
{
    EnumerateQuery *query = 0; // Private query instances

#pragma omp critical (CERR_OUTPUT)
{
    struct host_info hi = hosts.back();
    hosts.pop_back();
    int tnum = omp_get_thread_num();
    cerr << tnum << ": connecting to host_info " << hosts.size() << ": \"" << hi.name << "\" : " << hi.port << ", \"" << hi.enforcepath << "\"" << endl;

    /**
     * Initialize socket
     */
    if (verbose) cerr << "Init socket to " << hi.name << ":" << hi.port << endl;
    ClientSocket *cs = new ClientSocket(hi.name, hi.port);
    if (verbose) cerr << "Socket connection succeeded, sending the header \"" << libname(indexfile) << "\"" << endl;
    cs->putc('S'); // Start byte
    cs->putstring(libname(indexfile));
    if (verbose) cerr << "Header sent successfully." << endl;

    query = new EnumerateQuery(tc, *outputw, verbose, cs, hi.enforcepath, fmin, maxdepth);
    if (verbose) cerr << "Align mode: enumerate query with fmin = " << fmin << ", maxdepth = " << maxdepth << endl;
    assert(query != 0);
    // Other query settings
    query->setDebug(debug);

} // End of #pragma omp critical (CONSTRUCT_QUERY)

    /**
     * Main loop: iterate through input
     */
    unsigned reported = 0;
    query->enumerate(reported);

#pragma omp atomic
        total_found += reported ? 1 : 0;
#pragma omp atomic
        total_occs += reported;

    delete query;
} // end of #pragma omp parallel

    if (verbose)
    {	
        cerr << "Number of reported alignments: " << total_occs << endl;
        cerr << "Wall-clock time: " << std::difftime(time(NULL), wctime) << " seconds (" 
             << std::difftime(time(NULL), wctime) / 3600 << " hours)" << endl;

        struct rusage tval;
        getrusage(RUSAGE_SELF, &tval);
        cerr << "CPU time: " << tval.ru_utime.tv_sec << " seconds." << endl;
    }

    delete tc;
}
