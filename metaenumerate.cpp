#include "Query.h"
#include "OutputWriter.h"
#include "EnumerateQuery.h"

#include <sstream>
#include <iostream>
using std::cout;
using std::endl;
using std::cerr;
#include <string>
using std::string;
#include <ctime>
#include <cstring>
#include <getopt.h>
#include <sys/resource.h>

#ifdef PARALLEL_SUPPORT
#include <omp.h>
#endif

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
    cerr << "usage: " << name << " [options] <index> <hostname>" << endl
         << "Check README or `" << name << " --help' for more information." << endl;
}

void print_help(char const *name)
{
    cerr << "usage: " << name << " [options] <index> <hostname>" << endl
         << endl
         << " <index>        Index file." <<endl
         << " <hostname>     Host to connect to." <<endl <<endl
         << "Options: " <<endl
         << " --check        Check integrity of index and quit." << endl
         << " --port <p>     Connect to port <p>." <<endl
         << " --verbose      Print progress information." << endl
         << "Debug options:"<<endl
         << " --debug        Print more progress information." << endl
         << " --path <p>     Enumerate only below path p." << endl;
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
    for (const char *c = Query::ALPHABET_DNA; c < Query::ALPHABET_DNA + Query::ALPHABET_SIZE; ++c) 
    {
        ulong nmin = tc->LF(*c, 0lu - 1);
        ulong nmax = tc->LF(*c, n-1)-1;    
        if (nmax >= nmin)
        {
            if (verbose) cerr << *c << ": " << nmax-nmin+1 << endl;
            totals += nmax - nmin + 1;
        }
        else
        {
            cout << "FAILED *********** ";
            cerr << "FAILED *********** ";
            exit(1);
        }
    }

    ulong nmin = tc->LF('N', 0lu-1);
    ulong nmax = tc->LF('N', n-1)-1;    
    if (nmax >= nmin)
        totals += nmax - nmin + 1;
    if (verbose) cerr << "N: " << nmax-nmin+1 << endl;

    nmin = tc->LF(0, 0lu-1);
    nmax = tc->LF(0, n-1)-1;    
    if (nmax >= nmin)
        totals += nmax - nmin + 1;
    if (verbose) cerr << "0: " << nmax-nmin+1 << endl;

    nmin = tc->LF('.', 0lu-1);
    nmax = tc->LF('.', n-1)-1;    
    if (nmax >= nmin)
        totals += nmax - nmin + 1;
    if (verbose) cerr << ".: " << nmax-nmin+1 << endl;

    if (n == totals)
        cerr << "OK     ";
    else
        cerr << "FAILED *********** ";

    cerr  << std::showbase << "n = " << n << ", total = " << totals << ", number of . = " << nmax-nmin+1 << endl;
    cout << n << endl;
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
    string enforcepath = "";
    int portno = 54666;
    bool checkonly = false;
    bool debug = false;
    bool verbose = false;
#ifdef PARALLEL_SUPPORT
    unsigned parallel = 1;
#endif

    static struct option long_options[] =
        {
            {"fmin",      required_argument, 0, 'f'},
            {"maxdepth",  required_argument, 0, 'M'},
            {"path",      required_argument, 0, 'D'}, // for debuging
            {"port",      required_argument, 0, 'p'},
            {"check",     no_argument,       0, 'C'},
            {"parallel",  required_argument, 0, 'P'},
            {"verbose",   no_argument,       0, 'v'},
            {"help",      no_argument,       0, 'h'},
            {"debug",     no_argument,       0, long_opt_debug},
            {0, 0, 0, 0}
        };
    int option_index = 0;
    int c;
    while ((c = getopt_long(argc, argv, "f:M:D:p:CP:vh",
                                 long_options, &option_index)) != -1) 
    {
        switch(c) 
        {
        case 'f':
            fmin = atoi_min(optarg, 1, "-f, --fmin", argv[0]); break;
        case 'M':
            maxdepth = atoi_min(optarg, 75, "-M, --maxdepth", argv[0]); break;
        case 'D':
            enforcepath = string(optarg); break;
        case 'p':
            portno = atoi_min(optarg, 1024, "-p, --port", argv[0]); break;
        case 'C':
            checkonly = true;
            break;
        case 'P':
#ifdef PARALLEL_SUPPORT
            parallel = atoi_min(optarg, 0, "-P, --parallel", argv[0]); break;
#else
            cerr << "metaenumerate: Parallel processing not currently available!" << endl 
                 << "Please recompile with parallel support; see README for more information." << endl;
            return 1;
#endif
        case 'v':
            verbose = true;
            break;
        case long_opt_debug:
            debug = true; break;
        case '?': 
        case 'h':
            print_help(argv[0]);
            return 1;
        default:
            print_usage(argv[0]);
            std::abort ();
        }
    }

    // Parse filenames
    if (argc - optind != 2)
    {
        cerr << argv[0] << ": expecting one filename plus a hostname" << endl;
        print_usage(argv[0]);
        return 1;
    }

    string indexfile = string(argv[optind++]);
    string hostname = string(argv[optind++]);

    if (!enforcepath.empty())
        cerr << endl << "WARNING: Enforcing path: " << enforcepath << endl << endl;
    
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

#ifdef PARALLEL_SUPPORT
    if (parallel != 0)
    {
        if (verbose && parallel == 1) 
            cerr << "Using only one core (default setting, see option -P, --parallel)" << endl;
        if (verbose && parallel != 1)
            cerr << "Using " << parallel << " cores." << endl;
        omp_set_num_threads(parallel);
    }
    else
        if (verbose) cerr << "Using all " << omp_get_max_threads() << " cores available." << endl;
#pragma omp parallel 
#endif    
{
    EnumerateQuery *query = 0; // Private query instances

#ifdef PARALLEL_SUPPORT
#pragma omp critical (CONSTRUCT_QUERY)
#endif    
{
    /**
     * Initialize socket
     */
    if (verbose) cerr << "Init socket to " << hostname << ":" << portno << endl;
    ClientSocket *cs = new ClientSocket(hostname, portno);
    if (verbose) cerr << "Socket connection succeeded, sending the header \"" << libname(indexfile) << "\"" << endl;

    cs->putc('S'); // Start byte
    cs->putstring(libname(indexfile));
    if (verbose) cerr << "Header sent successfully." << endl;

    query = new EnumerateQuery(tc, *outputw, verbose, cs, enforcepath, fmin, maxdepth);
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

#ifdef PARALLEL_SUPPORT
#pragma omp atomic
        total_found += reported ? 1 : 0;
#pragma omp atomic
        total_occs += reported;
#else
        total_found += reported ? 1 : 0;
        total_occs += reported;
#endif

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
