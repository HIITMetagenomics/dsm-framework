// Converts a string mining result from <stdin>
// to three matrices that
//     1) count the number of samples that share the same substring.
//     2) sum_i [ log(1 + s_i) - log(1 + t_i) ]^2 
//     3) sum_i [ sqrt(s_i) - sqrt(t_i) ]^2
//     4) \sum_i [\lngamma(s_i + t_i + 1) - \lngamma(s_i + 1)
//                 - \lngamma(t_i + 1) - (s_i + t_i + 1)]
//
// Entropy is computed as H = - \sum_i p_i log p_i
// where 
//     p_i = (n_i + 1) / (\sum_j n_j + d)
//
// where n_i is the frequency in i'th sample, and d is the total number of samples.
// Finally, we divide with the maximum entropy to normalize the final value.
//
// TODO: The input is read row by row (fgets). It would be faster to read larger chuncks.
//
// -S,--samplefile file format is new-line separated list of integers of
// <run id> to <sample id> mappings, where <sample id> is the integer given 
// at row number <run id>. Both numberings start from 0.
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <getopt.h>
#include <math.h>

typedef unsigned long ulong;
#define DEBUG 0      // Can print a lot to stderr, keep it disabled.
#define ROWLEN 10000
#define MAXSMPLS 220 // Compile time setting, which can be set much larger

// Precomputed log, sqrt, lgamma for [0, PRECMP)
#define PRECMP 100000

double prelog[PRECMP], presqrt[PRECMP], prelgamma[PRECMP];
double *prenormlog[MAXSMPLS], *prenormsqrt[MAXSMPLS]; 

// Accessor for 3D array
#define OFFSET(x,y,z) ((x) * smpls * smpls + (y) * smpls + z)

// Static array to track samples 
unsigned samples[MAXSMPLS];
unsigned freq[MAXSMPLS];
double nfactor[MAXSMPLS]; // Normalization factors
int verbose = 0;          // Prints helpful information to stderr.
unsigned minfreq = 0;

struct parameters
{
    double maxent; // Maximum entropy (after dividing by max. entropy)
    unsigned noutput;
};

typedef struct _mymatrix
{
    unsigned count;
    double log;
    double sqrt;
    double lgamma;
} mymatrix;

int mycmp(const void *a, const void *b)
{
  return *(unsigned *)a - *(unsigned *)b;
}
int myparamcmp(const void *a, const void *b)
{
    struct parameters *x = (struct parameters *)a;
    struct parameters *y = (struct parameters *)b;
    if (x->maxent < y->maxent)
        return +1;
    return -1;
}
void myerror(const char *text, char const *name)
{
    fprintf(stderr, "\nerror: %s\nPlease see `%s --help'\n", text, name);
    abort();
}

// Parse one row of input values, and update the arrays freq and samples.
unsigned parse(char const *values, int runs, int *runtosmpl)
{
    char const *tmp = values;
    unsigned l = 0;
    while (*tmp)
    {
        unsigned run = atoi(tmp);
        while (*tmp && *tmp != ':') ++tmp;
        assert(*tmp == ':');
        unsigned frq = atoi(tmp+1);
        while (*tmp && *tmp != ' ') ++tmp;

        if (run >= runs)
        {
            fprintf(stderr, "error: expecting at most %d samples/runs, please increase the argument [smpls] "\
                    "and/or check the --samplefile\n", runs);
            exit(1);
        }

        if (frq < minfreq)
            continue;
        if (runtosmpl)
            run = runtosmpl[run];
        samples[l++] = run;
        freq[run] = frq;
    }
    assert(l <= MAXSMPLS);
    if (l == 0)
        return 0;

    // Sort & remove duplicate sample numbers
    qsort(samples, l, sizeof(unsigned), mycmp);
    unsigned j = 0, k = 1;
    while (k < l)
    {
        while (k < l && samples[k] == samples[j])
            ++k;
        if (k < l)
            samples[++j] = samples[k];
    }
    assert(j+1 <= l);
    return (j + 1); // Returns the number of unique sample id's
}

double entropy(unsigned l, int smpls)
{
    unsigned i;
    unsigned sumN = smpls;  // used to compute \sum_j n_j + d*1
    double sumNlogN = 0;   // used to compute \sum_i (n_i+1) log (n_i+1)
 
    // Update entropy
    for (i = 0; i < l; ++i)
    {
        unsigned frq = freq[samples[i]];
        sumN += frq;
        sumNlogN += (double)(frq+1) * log(frq+1)/log(2);
    }
    double entropy = (log(sumN)/log(2) - sumNlogN/(double)sumN);

    return log(2)*entropy/log(smpls); // Return entropy divided by max entropy.
}

double normalized_entropy(unsigned l, int smpls)
{
    unsigned i;
    double sumN = (double)smpls;  // used to compute \sum_j n_j + d*1
    double sumNlogN = 0;          // used to compute \sum_i (n_i+1) log (n_i+1)
 
    // Update entropy
    for (i = 0; i < l; ++i)
    {
        double frq = (double)freq[samples[i]] * nfactor[samples[i]]; // Normalize here
        sumN += frq;
        sumNlogN += (frq+1) * log(frq+1)/log(2);
    }
    double entropy = (log(sumN)/log(2) - sumNlogN/sumN);

    return log(2)*entropy/log(smpls); // Return entropy divided by max entropy.
}


void add(mymatrix *matrix, int smpls, unsigned l, int i)
{
    unsigned j,k;
    for (j = 0; j < l; ++j)
        for (k = j; k < l; ++k)
	    matrix[OFFSET(i,samples[j],samples[k])].count++;

    for (j = 0; j < smpls; ++j)
        for (k = j + 1; k < smpls; ++k)
            if (freq[j] || freq[k])
            {
                if (freq[j] < PRECMP && freq[k] < PRECMP) // The most common case is this one
                {
                    unsigned offset = OFFSET(i,j,k);
                    matrix[offset].log    += pow( prelog[freq[j]]  - prelog[freq[k]],  2);
                    matrix[offset].sqrt   += pow( presqrt[freq[j]] - presqrt[freq[k]], 2);
                    matrix[offset].lgamma += (freq[j] + freq[k] < PRECMP ? prelgamma[freq[j] + freq[k]] : lgamma(freq[j] + freq[k] + 1))
                        - prelgamma[freq[j]] - prelgamma[freq[k]] - (freq[j] + freq[k] + 1);
                }
                else
                {
                    unsigned offset = OFFSET(i,j,k);  // Rare case in our data
                    matrix[offset].log    += pow( log(1+freq[j]) - log(1+freq[k]), 2);
                    matrix[offset].sqrt   += pow( sqrt(freq[j])  - sqrt(freq[k]),  2);
                    matrix[offset].lgamma += lgamma(freq[j] + freq[k] + 1) - lgamma(freq[j] + 1) 
                                           - lgamma(freq[k] + 1) - (freq[j] + freq[k] + 1);
                }
            }
}

// FIXME lgamma disabled for now
void add_normalized(mymatrix *matrix, int smpls, unsigned l, int i)
{
    unsigned j,k;
    for (j = 0; j < l; ++j)
        for (k = j; k < l; ++k)
	    matrix[OFFSET(i,samples[j],samples[k])].count++;

    for (j = 0; j < smpls; ++j)
        for (k = j + 1; k < smpls; ++k)
            if (freq[j] || freq[k])
            {
                if (freq[j] < PRECMP && freq[k] < PRECMP) // The most common case is this one
                {
                    unsigned offset = OFFSET(i,j,k);
                    matrix[offset].log    += pow( prenormlog[j][freq[j]] - prenormlog[k][freq[k]],  2);
                    matrix[offset].sqrt   += pow(prenormsqrt[j][freq[j]] - prenormsqrt[k][freq[k]], 2);
                    /*matrix[offset].lgamma += (freq[j] < PRECMPLGAMMA && freq[k] < PRECMPLGAMMA 
                                              ? predoublelgamma[j][k][freq[j]][freq[k]]
                                              : lgamma((double)freq[j]*nfactor[j] + (double)freq[k]*nfactor[k] + 1))
                        - prenormlgamma[j][freq[j]] 
                        - prenormlgamma[k][freq[k]] 
                        - ((double)freq[j]*nfactor[j] + (double)freq[k]*nfactor[k] + 1);*/
                }
                else
                {
                    unsigned offset = OFFSET(i,j,k);  // Rare case in our data
                    matrix[offset].log    += pow( log(1+(double)freq[j]*nfactor[j]) - log(1+(double)freq[k]*nfactor[k]), 2);
                    matrix[offset].sqrt   += pow( sqrt( (double)freq[j]*nfactor[j]) - sqrt( (double)freq[k]*nfactor[k]),  2);
                    /*matrix[offset].lgamma += lgamma((double)freq[j]*nfactor[j] + (double)freq[k]*nfactor[k] + 1) 
                        - lgamma((double)freq[j]*nfactor[j] + 1)
                        - lgamma((double)freq[k]*nfactor[k] + 1) 
                        -       ((double)freq[j]*nfactor[j] + (double)freq[k]*nfactor[k] + 1);*/
                }
            }
}

// Accumulate the counts of longer strings over to shorter string lengths.
void accumulate(mymatrix *matrix, int i, int j, int smpls)
{
    int h, k;
    for (h = 0; h < smpls; ++h)
        for (k = 0; k < smpls; ++k)
        {
            matrix[OFFSET(j,h,k)].count += matrix[OFFSET(i,h,k)].count;
            matrix[OFFSET(j,h,k)].log += matrix[OFFSET(i,h,k)].log;
            matrix[OFFSET(j,h,k)].sqrt += matrix[OFFSET(i,h,k)].sqrt;
            matrix[OFFSET(j,h,k)].lgamma += matrix[OFFSET(i,h,k)].lgamma;
        }
}

int atoi_min(char const *value, int min, char const *parameter, char const *name)
{
    int i = atoi(value);
    if (i < min)
    {
        fprintf(stderr, "%s: argument %s must be greater than or equal to %d\n", name, parameter, min);
        fprintf(stderr, "Check README or `%s --help' for more information.\n", name);
        exit(1);
    }
    return i;
}

double * parse_entropy_steps(char *value, int *n, double min, double max, char const *parameter, char const *name)
{
    // Allocate space
    double step = atof(value);
    if (step <= min || step > max)
    {
        fprintf(stderr, "%s: arguments of %s must be between %f and %f\n", name, parameter, min, max);
        fprintf(stderr, "Check README or `%s --help' for more information.\n", name);
        exit(1);
    }
    *n = round(1/step+0.5);
    if (((*n)-1) * step < 1.0)
        ++(*n);
    double *doubles = (double *)malloc((*n) * sizeof(double));
    double sum = 0;
    int i = 0;
    while (i < (*n)-1)
    {
        doubles[i] = sum;
        sum += step;
        ++i;
    }
    doubles[i] = 1.0;
    return doubles;
}

double * parse_doubles(char *value, int *n, double min, double max, char const *parameter, char const *name)
{
    // Allocate space
    *n = 1;
    char *pch = value;
    while (*pch)
    {
        if (*pch == ' ' || *pch == ',' || *pch == '-')
            ++(*n);
        ++pch;
    }
    double *doubles = (double *)malloc((*n) * sizeof(double));
    *n = 0;
    pch = strtok(value," ,-");
    while (pch != NULL)
    {
        doubles[*n] = atof(pch);
        if (doubles[*n] < min || doubles[*n] > max)
        {
            fprintf(stderr, "%s: arguments of %s must be between %f and %f\n", name, parameter, min, max);
            fprintf(stderr, "Check README or `%s --help' for more information.\n", name);
            exit(1);
        }
        ++(*n);
        pch = strtok (NULL, " ,-");
    }

    return doubles;
}

int * parse_ints(char *value, int *n, int min, char const *parameter, char const *name)
{
    // Allocate space
    *n = 1;
    char *pch = value;
    while (*pch)
    {
        if (*pch == ' ' || *pch == ',' || *pch == '.' || *pch == '-')
            ++(*n);
        ++pch;
    }
    int *ints = (int *)malloc((*n) * sizeof(int));
    *n = 0;
    pch = strtok(value," ,.-");
    while (pch != NULL)
    {
        ints[*n] = atoi(pch);
        if (ints[*n] < min)
        {
            fprintf(stderr, "%s: arguments of %s must be greater than or equal to %d\n", name, parameter, min);
            fprintf(stderr, "Check README or `%s --help' for more information.\n", name);
            exit(1);
        }
        ++(*n);        
        pch = strtok (NULL, " ,.-");
    }

    return ints;
}

void longhelp(char const *name)
{
    fprintf(stderr, "usage: %s <options>\n", name);
    fprintf(stderr, "where mandatory options are:\n\t-s,--samples [smpls]\tNumber of samples in the input data.\n" \
            "\t\t\t\tOutput matrices are of size [smpls X smpls].\n");
    fprintf(stderr, "\t-S,--samplefile [file]\tFile containing runs to samples mapping.\n" \
            "\t\t\t\tGive either -s,--samples or -S,--samplefile, but not both.\n");
    fprintf(stderr, "\t-m,--maxent [maxe1],<maxe2>,...,<maxeK>\n"\
            "\t\t\t\tComma separated list of maximum entropy for K matrices.\n" \
            "\t\t\t\tThe values must be between 0 and 1, and are compared\n" \
            "\t\t\t\tagainst entropy divided by max. entropy.\n" \
            "\t\t\t\tOutput is H times K matrices.\n");
    fprintf(stderr, "\t-e,--entstep <step>\t"\
            "Entropy <step> to consider all values (0;step;1).\n" \
            "\t\t\t\tGive either -m,--maxent or -e,--entstep, but not both.\n");
    fprintf(stderr, "\t-F,--file <filename>\tSuffix for output files.\n");
    fprintf(stderr, "Optional options are:\n");
    fprintf(stderr, "\t-M,--minfreq <min>\t\tMin. freq. value to consider.\n");
    fprintf(stderr, "\t-N,--normalize <file>\t\tNormalize the freq's. \n\t\t\t\t<file> contains the dataset sizes.\n");
    fprintf(stderr, "\t-v,--verbose\t\tVerbose mode (stderr).\n");
}

FILE * open_output_file(char const *suffix, char const *type)
{
    char file[ROWLEN];
    snprintf(file, ROWLEN, "%s.%s", type, suffix);
    FILE *f = fopen(file, "r");
    if (f != NULL)
    {     // File already exists.
        fprintf(stderr, "error: output file %s already exists. Aborting!\n", file);
        exit(1);
    }

    f = fopen(file, "w");
    if (f == NULL)
    {
        fprintf(stderr, "error: could not open output file %s. Aborting!\n", file);
        exit(1);
    }
    return f;
}

int parse_samples_file(char const *filename, int **runtosmpl, int *smpls)
{
    FILE *f = fopen(filename, "rt");
    if (f == NULL)
    { 
        fprintf(stderr, "error: sample file -S,--samplefile %s does not exist. Aborting!\n", filename);
        exit(1);
    }
    int n = 0, i;
    char row[ROWLEN];
    while (!feof(f))
    {
        if (fgets(row, ROWLEN, f) == NULL)
            break;
        if (strlen(row) == 0)
            continue;
        ++n;
    }

    // Rewind
    fseek(f, 0, SEEK_SET);
    int *rts = (int *)malloc(n*sizeof(int));
    for (i = 0; i < n; ++i)
        rts[i] = -1;
    n = 0;
    *smpls = 0;
    while (!feof(f))
    {
        if (fscanf(f, "%d\n", &rts[n]) != 1)
        {
            fprintf(stderr, "error: unable to parse the file -S,--samplefile %s. Aborting!\n", filename);
            abort();
        }
        assert(rts[n] != -1);
        assert(rts[n] < MAXSMPLS);
        if (rts[n]+1 > *smpls)
            *smpls = rts[n]+1;
        ++n;
    }
    *runtosmpl = rts;
    fclose(f);
    return n;
}

double * parse_size_file(char const * filename, int runs)
{
    FILE *f = fopen(filename, "rt");
    if (f == NULL)
    { 
        fprintf(stderr, "error: size file for -N,--normalize %s does not exist. Aborting!\n", filename);
        exit(1);
    }

    double *dsizes = (double *)malloc(runs * sizeof(double));
    int i = 0;
    char tmp[100];
    while (!feof(f) && i < runs)
    {
        if (fscanf(f, "%100s\t%lf\n", tmp, &dsizes[i]) != 2)
        {
            fprintf(stderr, "error: unable to parse the file -N,--normalize %s. Aborting!\n", filename);
            abort();
        }
        ++i;
    }
    if (i != runs)
    {
        fprintf(stderr, "error: unable to parse the file -N,--normalize %s. Received %d values when expecting %d values. Aborting!\n", filename, i, runs);
        abort();
    }
    fclose(f);
    return dsizes;
}

int main(int argc, char **argv) 
{
    int i;
    // init static stuff
    for (i = 0; i < MAXSMPLS; ++i)
        freq[i] = 0;
    for (i = 0; i < PRECMP; ++i)
    {
        prelog[i] = log(i+1);
        presqrt[i] = sqrt(i);
        prelgamma[i] = lgamma(i+1);
    }

    if (argc < 5)
    {
        longhelp(argv[0]);
        return 1;
    }

    int parsepvalues = 0;
    int normalize = 0;
    int smpls = -1, runs = -1;
    int *runtosmpl = 0;
    double *maxent = 0;
    int nmaxent = 0;
    char filesuffix[ROWLEN];
    char smplsfile[ROWLEN];
    char normfile[ROWLEN];
    filesuffix[0] = 0; // empty
    smplsfile[0] = 0;
    normfile[0] = 0;

    static struct option long_options[] =
        {
            {"samples",      required_argument, 0, 's'},
            {"samplefile",   required_argument, 0, 'S'},
            {"maxent",       required_argument, 0, 'm'},
            {"entstep",      required_argument, 0, 'e'},
            {"file",         required_argument, 0, 'F'},            
            {"normalize",    required_argument, 0, 'N'},
            {"minfreq",      required_argument, 0, 'M'},
            {"verbose",      no_argument,       0, 'v'},
            {"help",         no_argument,       0, 'h'},
            {0, 0, 0, 0}
        };
    int option_index = 0;
    int c;
    while ((c = getopt_long(argc, argv, "s:S:m:e:F:N:M:vh", long_options, &option_index)) != -1) 
    {
        switch(c) 
        {
        case 's':
            smpls = atoi_min(optarg, 2, "-s,--samples", argv[0]); 
            break;
        case 'S':
            strncpy(smplsfile, optarg, ROWLEN);
            break;
        case 'm':
            assert(maxent == 0);
            maxent = parse_doubles(optarg, &nmaxent, 0.0, 1.0, "-m,--maxent", argv[0]);
            break;
        case 'e':
            assert(maxent == 0);
            maxent = parse_entropy_steps(optarg, &nmaxent, 0.0, 1.0, "-e,--entstep", argv[0]);
            break;
        case 'F':
            strncpy(filesuffix, optarg, ROWLEN);
            break;
        case 'N':
            normalize = 1;
            strncpy(normfile, optarg, ROWLEN);
            break;
        case 'M':
            minfreq = atoi_min(optarg, 1, "-M,--minfreq", argv[0]); 
            break;
        case 'v':
            verbose = 1; 
            break;
        case 'h':
            longhelp(argv[0]); 
            return 1;
        default:
            myerror("invalid command line argument given!? Please check `%s --help'\n", argv[0]);
            return 1;
            break;
        }
    }

    if (argc != optind)
        fprintf(stderr, "warning: ignoring the last %d arguments\n", argc-optind);

    // sanity checks
    if (filesuffix[0] == 0)
        myerror("the argument -F,--file is mandatory.", argv[0]);
    if (nmaxent < 1)
        myerror("the argument -m,--maxent is mandatory.", argv[0]);
    if (smpls == -1 && strlen(smplsfile) == 0)
        myerror("give either the argument -s,--samples or -S,--samplefile.", argv[0]);
    if (smpls != -1 && strlen(smplsfile))
        myerror("both the arguments -s,--samples and -S,--samplefile cannot be given at the same time.", argv[0]);
    if (smpls != -1 && smpls < 2)
        myerror("the argument -s,--samples must be at least 2.", argv[0]);
    if (nmaxent == 0)
        myerror("the argument -m,--maxent is mandatory.", argv[0]);

    // Parse samples file if needed
    if (smpls == -1)
    {
        assert(strlen(smplsfile));
        runs = parse_samples_file(smplsfile, &runtosmpl, &smpls);
        if (verbose) fprintf(stderr, "sample file: got mapping from %d runs to %d samples\n", runs, smpls);
        if (smpls < 2 || runs < smpls)
            myerror("unable to parse the samples file in the argument -S,--samplefile.", argv[0]);
    }
    else
        runs = smpls;

    if (runs > MAXSMPLS)
    {
        fprintf(stderr, "error: expecting at most %d samples/runs, please increase MAXSMPLS in the source code and recompile\n", MAXSMPLS);
        abort();
    }

    // Parse normalization factors
    double *dsizes = 0;
    if (normalize || normfile[0])
    {
        assert (normalize);
        dsizes = parse_size_file(normfile, smpls);
        assert(dsizes != 0);

        // Compute norm. factors
        assert (smpls < MAXSMPLS);
        for (i = 0; i < smpls; ++i)
        {
            // Iterates over samples to precompute
            assert(dsizes[i] != 0);
            nfactor[i] = (double)1/dsizes[i];

            // Init precomputed values
            prenormlog[i] =    (double *)malloc(PRECMP * sizeof(double));
            prenormsqrt[i] =   (double *)malloc(PRECMP * sizeof(double));
//            prenormlgamma[i] = (double *)malloc(PRECMP * sizeof(double));
            int j;
            for (j = 0; j < PRECMP; ++j)
            {
                // i == sample, j == frequency
                prenormlog[i][j] = log((double)j*nfactor[i] + 1);
                prenormsqrt[i][j] = sqrt((double)j*nfactor[i]);
//                prenormlgamma[i][j] = lgamma((double)j*nfactor[i] + 1); 
            }
        }
        if (verbose) fprintf(stderr, "normalization file %s loaded\n", normfile);
        free(dsizes);
    }

    // Init the matrices
    i = 0;
    int j, nmatrices = nmaxent;
    struct parameters *param = (struct parameters *)malloc(nmatrices * sizeof(struct parameters));
    for (j = 0; j < nmaxent; ++j)
    {
        param[i].maxent = maxent[j];
        param[i].noutput = 0;
        ++i;
    }
    FILE *fileCount = open_output_file(filesuffix, "count");
    FILE *fileLog = open_output_file(filesuffix, "log");
    FILE *fileSqrt = open_output_file(filesuffix, "sqrt");
    FILE *fileLgamma = open_output_file(filesuffix, "lgamma");

    assert(i == nmatrices);
    qsort(param, nmatrices, sizeof(struct parameters), myparamcmp);

    if (verbose)
    {
        fprintf(stderr, "Computing %d matrices for <max_entropy> values:", nmatrices);
        for (i = 0; i < nmatrices; ++i)
            fprintf(stderr, (i == 0 ? " <%f>" : ", <%f>"), param[i].maxent);
        fprintf(stderr, "\n"); 
    }

    // Init matrices
    mymatrix *matrix = (mymatrix *)malloc(nmatrices * smpls * smpls * sizeof(mymatrix));
    for (i = 0; i < nmatrices * smpls * smpls; ++i)
    {
        matrix[i].count = 0;
        matrix[i].log = 0;
        matrix[i].sqrt = 0;
        matrix[i].lgamma = 0;
    }        

    // Init parsing
    time_t wctime = time(NULL);    
    char row[ROWLEN];
    unsigned long rowno = 0;
    while (!feof(stdin))
    {
        if (fgets(row, ROWLEN, stdin) == NULL)
            break;

        char *tmp = row; 
        while (*tmp && *tmp != ' ') ++tmp; // Finds the first ' ',
        assert(*tmp == ' ');
        // Check if we need to parse p-values
        if (rowno == 0)
        {
            char *t = tmp;
            while (*t && *t != '.') ++t;
            if (*t == '.')
                parsepvalues = 1;
            else
                assert(parsepvalues == 0);
        }
        if (parsepvalues)
        {
            //double entropy = atof(tmp++);  // FIXME We need to recompute the entropy if runtosmpl mapping is set
            ++tmp;
            while (*tmp && *tmp != ' ') ++tmp; // Finds the second ' ',
            assert(*tmp == ' ');
        }

        // Parse row
        unsigned uniqueids = parse(tmp, runs, runtosmpl);
        // Retrieve the difference to max. entropy
        double entr = -1;
        if (normalize)
            entr = normalized_entropy(uniqueids, smpls);
        else
            entr = entropy(uniqueids, smpls);

        // Update correct matrix (note: only one matrix needs to be updated, values are accumulated later)
        for (i = nmatrices; i > 0;)
        {
            --i;
            if (entr <= param[i].maxent)
            {
                param[i].noutput ++;
                if (normalize)
                    add_normalized(matrix, smpls, uniqueids, i);
                else
                               add(matrix, smpls, uniqueids, i);
                
                break;
            }
        }
        // Zero the vector
        for (i = 0; i < uniqueids; ++i)
            freq[samples[i]] = 0;
        
        rowno++;
        if (verbose && rowno % 1000000 == 0)
        {
            fprintf(stderr, "Reading row %lu (%.5s...). Time: %.0f s (%.2f hours)\n", rowno, row, difftime(time(NULL), wctime), difftime(time(NULL), wctime) / 3600);
            fprintf(stderr, "noutput values: ");
            int j;
            for (j = 0; j < nmatrices; ++j)
                fprintf(stderr, " %u", param[j].noutput);
            fprintf(stderr, "\n");
        }
    }

    for (i = 0; i < MAXSMPLS; ++i)
        assert(freq[i] == 0);

    // Print output matrices
    for (i = nmatrices; i > 0;)
    {
        --i;
        fprintf(fileCount,  "Matrix for <max_entropy>=<%f> was computed from %u substrings: \n", param[i].maxent, param[i].noutput);
        fprintf(fileLog,    "Matrix for <max_entropy>=<%f> was computed from %u substrings: \n", param[i].maxent, param[i].noutput);
        fprintf(fileSqrt,   "Matrix for <max_entropy>=<%f> was computed from %u substrings: \n", param[i].maxent, param[i].noutput);
        fprintf(fileLgamma, "Matrix for <max_entropy>=<%f> was computed from %u substrings: \n", param[i].maxent, param[i].noutput);
        
        int k;
        for (j = 0; j < smpls; ++j)
        {
            for (k = 0; k < smpls; ++k)
            {
                fprintf(fileCount,  " %u", matrix[OFFSET(i,j,k)].count);
                fprintf(fileLog,    " %f", matrix[OFFSET(i,j,k)].log);
                fprintf(fileSqrt,   " %f", matrix[OFFSET(i,j,k)].sqrt);
                fprintf(fileLgamma, " %f", matrix[OFFSET(i,j,k)].lgamma);
            }
            fprintf(fileCount, "\n");
            fprintf(fileLog, "\n");
            fprintf(fileSqrt, "\n");
            fprintf(fileLgamma, "\n");
        }

        // Accumulate counts from larger diffs
        if (i)
        {
            param[i-1].noutput += param[i].noutput;
            accumulate(matrix, i, i-1, smpls);        
        }
    }

    if (verbose)
    {
        fprintf(stderr, "Number of lines processed: %lu\n", rowno);
        fprintf(stderr, "Wall-clock time: %.0f s (%.2f hours)\n", difftime(time(NULL), wctime), difftime(time(NULL), wctime) / 3600);
    }

    fclose(fileCount);
    fclose(fileLog);
    fclose(fileSqrt);
    fclose(fileLgamma);
    free(param);
    free(maxent);
    free(matrix);
    if (runtosmpl) 
        free(runtosmpl);
    if (normalize)
        for (i = 0; i < smpls; ++i)
        {
            free (prenormlog[i]);
            free (prenormsqrt[i]);
//            free (prenormlgamma[i]);
        }
    return 0;
}
