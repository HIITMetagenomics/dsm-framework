#include <cstdlib>
#include <ctime>
#include <iostream>
#include <vector>

#include "rlcsa.h"
#include "adaptive_samples.h"
#include "suffixarray.h"
#include "docarray.h"

#ifdef MULTITHREAD_SUPPORT
#include <omp.h>
#endif


using namespace CSA;


const int MAX_THREADS = 64;
const usint MAX_OCCURRENCES = WORD_MAX;



void
printUsage()
{
  std::cout << "Usage: rlcsa_test [options] base_name [patterns [threads]]" << std::endl;
  std::cout << "  -a   Use adaptive samples." << std::endl;
  std::cout << "  -d   Use direct locate / document listing." << std::endl;
  std::cout << "  -g#  Use the weights to generate # actual patterns." << std::endl;
  std::cout << "  -i#  Ignore first # characters of patterns." << std::endl;
  std::cout << "  -l   Locate the occurrences." << std::endl;
  std::cout << "  -L   List the documents containing the pattern." << std::endl;
  std::cout << "  -p   Pattern file is in Pizza & Chili format." << std::endl;
  std::cout << "  -S   Use a plain suffix array (negates adaptive, direct, steps)." << std::endl;
  std::cout << "  -s   Count the number of steps required for locate() (negates write)." << std::endl;
  std::cout << "  -W   Write the actual patterns into patterns.selected." << std::endl;
  std::cout << "  -w   Write the weighted number of occurrences of each suffix into patterns.found." << std::endl;
  std::cout << std::endl;
  std::cout << "Default pattern weight for -g and -w is 1." << std::endl;
  std::cout << "If -i is specified, the ignored characters are used as weights." << std::endl;
  std::cout << "Use -it to ignore the input until the first \\t." << std::endl;
  std::cout << "Option -W also writes the lexicographic ranges into patterns.ranges and" << std::endl;
  std::cout << "the occurrences/steps into patterns.occ." << std::endl;
  std::cout << std::endl;
}


int main(int argc, char** argv)
{
  std::cout << "RLCSA test" << std::endl;
  std::cout << std::endl;

  bool adaptive = false, direct = false, locate = false, pizza = false, count_steps = false, write = false, write_patterns = false;
  bool use_sa = false;
  bool listing = false;
  usint ignore = 0, generate = 0;
  bool ignore_tab = false;
  char* base_name = 0;
  char* patterns_name = 0;
  #ifdef MULTITHREAD_SUPPORT
  sint threads = 1;
  #endif
  for(int i = 1; i < argc; i++)
  {
    if(argv[i][0] == '-')
    {
      switch(argv[i][1])
      {
        case 'a':
          adaptive = true; break;
        case 'd':
          direct = true; break;
        case 'g':
          generate = atoi(argv[i] + 2); break;
        case 'i':
          if(argv[i][2] == 't') { ignore = 1; ignore_tab = true; }
          else                  { ignore = atoi(argv[i] + 2); }
          break;
        case 'l':
          locate = true; break;
        case 'L':
          listing = true; break;
        case 'p':
          pizza = true; break;
        case 'S':
          use_sa = true; break;
        case 's':
          count_steps = true; break;
        case 'W':
          write_patterns = true; break;
        case 'w':
          write = true; break;
        default:
          std::cout << "Invalid option: " << argv[i] << std::endl << std::endl;
          printUsage();
          return 1;
      }
    }
    else if(base_name == 0)
    {
      base_name = argv[i];
    }
    else if(patterns_name == 0)
    {
      patterns_name = argv[i];
    }
    #ifdef MULTITHREAD_SUPPORT
    else
    {
      threads = std::min(MAX_THREADS, std::max(atoi(argv[i]), 1));
    }
    #endif
  }
  if(base_name == 0) { printUsage(); return 2; }

  std::cout << "Options:";
  if(generate > 0) { std::cout << " generate=" << generate; }
  if(ignore_tab) { std::cout << " ignore=tab"; }
  else if(ignore > 0) { std::cout << " ignore=" << ignore; }
  if(use_sa) { adaptive = direct = count_steps = listing = false; }
  if(listing)
  {
    locate = adaptive = count_steps = write = false;
    std::cout << " listing";
    if(direct) { std::cout << "(direct)"; }
  }
  else if(locate)
  {
    std::cout << " locate";
    if(adaptive || direct || count_steps)
    {
      std::cout << "(";
      if(adaptive)    { std::cout << " adaptive"; direct = true; }
      if(direct)      { std::cout << " direct"; }
      if(count_steps) { std::cout << " steps"; write = false; }
      std::cout << " )";
    }
  }
  else { adaptive = direct = count_steps = write = false; }
  if(pizza) { std::cout << " pizza"; }
  if(use_sa) { std::cout << " sa"; }
  if(write_patterns) { std::cout << " write_patterns"; }
  if(write) { std::cout << " write"; }
  std::cout << std::endl;
  std::cout << "Base name: " << base_name << std::endl;
  if(patterns_name != 0) { std::cout << "Patterns: " << patterns_name << std::endl; }
  #ifdef MULTITHREAD_SUPPORT
  std::cout << "Threads: " << threads << std::endl; 
  #endif
  std::cout << std::endl;

  const RLCSA* rlcsa = (use_sa ? 0 : new RLCSA(base_name, false));
  const SuffixArray* sa = (use_sa ? new SuffixArray(base_name, false) : 0);
  usint size = 0, text_size = 0;
  if(use_sa)
  {
    if(!sa->isOk())
    {
      delete rlcsa; rlcsa = 0;
      delete sa; sa = 0;
      return 3;
    }
    sa->reportSize(true);
    size = sa->getSize();
    text_size = size;
  }
  else
  {
    if(!rlcsa->isOk())
    {
      delete rlcsa; rlcsa = 0;
      delete sa; sa = 0;
      return 3;
    }
    rlcsa->printInfo();
    rlcsa->reportSize(true);
    size = rlcsa->getSize();
    text_size = rlcsa->getTextSize();
  }
  if(patterns_name == 0)
  {
    delete rlcsa; rlcsa = 0;
    delete sa; sa = 0;
    return 0;
  }

  AdaptiveSamples* adaptive_samples = 0;
  if(adaptive)
  {
    adaptive_samples = new AdaptiveSamples(*rlcsa, base_name);
    adaptive_samples->report();
    if(!adaptive_samples->isOk())
    {
      delete rlcsa; rlcsa = 0;
      delete sa; sa = 0;
      delete adaptive_samples; adaptive_samples = 0;
      return 4;
    }
  }

  DocArray* docarray = 0;
  if(listing)
  {
    docarray = new DocArray(base_name, *rlcsa);
    docarray->reportSize(true);
    if(!docarray->isOk() || !docarray->hasGrammar())
    {
      delete rlcsa; rlcsa = 0;
      delete sa; sa = 0;
      delete adaptive_samples; adaptive_samples = 0;
      delete docarray; docarray = 0;
      return 5;
    }
  }

  std::ifstream patterns(patterns_name, std::ios_base::binary);
  if(!patterns)
  {
    std::cerr << "Error opening pattern file!" << std::endl;
    delete rlcsa; rlcsa = 0;
    delete sa; sa = 0;
    delete adaptive_samples; adaptive_samples = 0;
    return 6;
  }

  std::vector<std::string> rows;
  if(!pizza) { readRows(patterns, rows, true); }
  else       { readPizzaChili(patterns, rows); }
  if(rows.size() == 0)
  {
    std::cerr << "No patterns found!" << std::endl;
    delete rlcsa; rlcsa = 0;
    delete sa; sa = 0;
    delete adaptive_samples; adaptive_samples = 0;
    return 7;
  }


  // Generate random patterns according to weights.
  if(generate > 0)
  {
    DeltaEncoder encoder(2 * sizeof(usint));
    usint sum = 0;
    for(usint i = 0; i < rows.size(); i++)
    {
      encoder.addBit(sum);
      if(ignore_tab)
      {
        usint pos = rows[i].find('\t');
        sum += atoi(rows[i].substr(0, pos).c_str());
      }
      else if(ignore > 0) { sum += atoi(rows[i].substr(0, ignore).c_str()); }
      else                { sum++; }
    }
    encoder.flush();
    DeltaVector vector(encoder, sum);

    std::vector<std::string> patterns; patterns.reserve(generate);
    DeltaVector::Iterator iter(vector);
    srand(0xDEADBEEF);  // FIXME random seed
    for(usint i = 0; i < generate; i++)
    {
      usint tmp = iter.rank(rand() % sum);
      patterns.push_back(rows[tmp - 1]);
    }
    rows.clear(); rows.reserve(generate);
    for(std::vector<std::string>::iterator iter = patterns.begin(); iter != patterns.end(); ++iter)
    {
      rows.push_back(*iter);
    }
    std::cout << "Generated " << rows.size() << " patterns." << std::endl;
    std::cout << std::endl;
  }
  
  // Prepare weights for writing the number of occurrences.
  weight_type* weights = 0;
  weight_type* totals = 0;
  if(write)
  {
    weights = new weight_type[rows.size()];
    totals = new weight_type[text_size];
    for(usint i = 0; i < rows.size(); i++)
    {
      if(ignore_tab)
      {
        usint pos = rows[i].find('\t');
        weights[i] = atoi(rows[i].substr(0, pos).c_str());
      }
      else if(ignore > 0) { weights[i] = atoi(rows[i].substr(0, ignore).c_str()); }
      else                { weights[i] = 1; }
    }
    for(usint i = 0; i < text_size; i++) { totals[i] = 0; }
  }

  // Replace weighted patterns with actual patterns.
  if(ignore_tab)
  {
    for(usint i = 0; i < rows.size(); i++)
    {
      usint pos = rows[i].find('\t');
      rows[i] = rows[i].substr(pos + 1);
    }
  }
  else if(ignore > 0)
  {
    for(usint i = 0; i < rows.size(); i++) { rows[i] = rows[i].substr(ignore); }
  }

  // Prepare to write the lexicographic ranges.
  std::ofstream* range_file = 0;
  std::ofstream* occ_file = 0;
  if(write_patterns)
  {
    std::string range_name(patterns_name); range_name += ".ranges";
    range_file = new std::ofstream(range_name.c_str(), std::ios_base::binary);
    if(!*range_file)
    {
      std::cerr << "Cannot open output file (" << range_name << ")!" << std::endl;
      delete range_file; range_file = 0;
    }
    else
    {
      pair_type temp = rlcsa->getSARange();
      range_file->write((char*)&temp, sizeof(temp));
    }
    std::string occ_name(patterns_name); occ_name += ".occ";
    occ_file = new std::ofstream(occ_name.c_str(), std::ios_base::binary);
    if(!*occ_file)
    {
      std::cerr << "Cannot open output file (" << occ_name << ")!" << std::endl;
      delete occ_file; occ_file = 0;
    }
  }


  // Actual work.
  usint total = 0, total_size = 0, ignored = 0, found = 0, steps = 0, total_docs = 0;
  #ifdef MULTITHREAD_SUPPORT
  omp_set_num_threads(threads);
  #endif
  double start = readTimer();

  #pragma omp parallel for schedule(dynamic, 1)
  for(usint i = 0; i < rows.size(); i++)
  {
    pair_type result;
    if(use_sa) { result = sa->count(rows[i]); }
    else       { result = rlcsa->count(rows[i]); }
    usint occurrences = length(result);
    #pragma omp critical
    {
      if(write_patterns)
      {
        range_file->write((char*)&result, sizeof(result));
      }
      if(!isEmpty(result))
      {
        found++;
        if(occurrences <= MAX_OCCURRENCES) { total += occurrences; }
        else { ignored++; }
      }
      total_size += rows[i].length();
    }
    if(locate && !isEmpty(result) && occurrences <= MAX_OCCURRENCES)
    {
      usint* matches = 0;
      uint* sa_matches = 0;
      if(adaptive)    { matches = adaptive_samples->locate(result, count_steps); }
      else if(use_sa) { sa_matches = sa->locate(result); matches = 0; }
      else            { matches = rlcsa->locate(result, direct, count_steps); } 
      #pragma omp critical
      {
        if(count_steps)
        {
          for(usint j = 0; j < occurrences; j++) { steps += matches[j]; }
        }
        else if(write)
        {
          if(use_sa) { for(usint j = 0; j < occurrences; j++) { totals[sa_matches[j]] += weights[i]; } }
          else       { for(usint j = 0; j < occurrences; j++) { totals[matches[j]] += weights[i]; } }
        }
        if(write_patterns)
        {
          if(use_sa) { occ_file->write((char*)sa_matches, occurrences * sizeof(uint)); }
          else       { occ_file->write((char*)matches, occurrences * sizeof(usint)); }
        }
      }
      delete[] sa_matches;
      delete[] matches;
    }
    if(listing && !isEmpty(result))
    {
      std::vector<usint>* docs = 0;
      if(direct) { docs = docarray->directListing(result); }
      else       { docs = docarray->listDocuments(result); }
      if(docs != 0)
      {
        #pragma omp critical
        {
          total_docs += docs->size();
          if(write_patterns)
          {
            for(std::vector<usint>::iterator iter = docs->begin(); iter != docs->end(); ++iter)
            {
              usint temp = *iter;
              occ_file->write((char*)&temp, sizeof(temp));
            }
          }
        }
      }
      delete docs;
    }
  }

  double time = readTimer() - start;
  double megabytes = total_size / (double)MEGABYTE;


  // Report.
  std::cout << "Patterns:      " << rows.size() << " (" << (rows.size() / time) << " / sec)" << std::endl;
  std::cout << "Total size:    " << megabytes << " MB";
  if(!locate)
  {
    std::cout << " (" << (megabytes / time) << " MB/s)";
  }
  std::cout << std::endl;
  std::cout << "Found:         " << found << std::endl;
  std::cout << "Occurrences:   " << total;
  if(locate)
  {
    std::cout << " (" << (total / time) << " / sec)";
    if(count_steps)
    {
      std::cout << std::endl
                << "Locate steps:  " << steps << " (" << (steps / (double)total) << " / occurrence)";
    }
  }
  std::cout << std::endl;
  if(listing)
  {
    std::cout << "Documents:     " << total_docs << " (" << (total_docs / time) << " / sec)" << std::endl;
  }
  std::cout << "Time:          " << time << " seconds" << std::endl;
  std::cout << std::endl;
  if(ignored > 0)
  {
    std::cout << "Ignored  " << ignored << " patterns with more than " << MAX_OCCURRENCES << " occurrences." << std::endl;
    std::cout << std::endl;
  }


  // Write.
  if(write)
  {
    std::string output_name(patterns_name); output_name += ".found";
    std::ofstream output(output_name.c_str(), std::ios_base::binary);
    if(!output)
    {
      std::cerr << "Cannot open output file (" << output_name << ")!" << std::endl;
    }
    else
    {
      output.write((char*)totals, text_size * sizeof(weight_type));
      output.close();
    }

    sint max_weight = 0, total_weight = 0;
    for(usint i = 0; i < size; i++) 
    {
      max_weight = std::max(max_weight, (sint)totals[i]);
      total_weight += totals[i];
    }
    std::cout << "Weights:       " << max_weight << " (max), " 
              << (((double)total_weight) / size) << " (average)" << std::endl;
    std::cout << std::endl;
  }

  if(write_patterns)
  {
    std::string output_name(patterns_name); output_name += ".selected";
    std::ofstream output(output_name.c_str(), std::ios_base::binary);
    if(!output)
    {
      std::cerr << "Cannot open output file (" << output_name << ")!" << std::endl;
    }
    else
    {
      for(usint i = 0; i < rows.size(); i++)
      {
        output << rows[i] << std::endl;
      }
      output.close();
    }
    delete range_file; range_file = 0;
    delete occ_file; occ_file = 0;
  }


  // Cleanup.

  if(adaptive) { adaptive_samples->report(); }
  delete rlcsa; rlcsa = 0;
  delete sa; sa = 0;
  delete adaptive_samples; adaptive_samples = 0;
  delete docarray; docarray = 0;
  delete[] weights; weights = 0;
  delete[] totals; totals = 0;
  return 0;
}
