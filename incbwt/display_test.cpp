#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>

#include "rlcsa.h"
#include "misc/utils.h"

#ifdef MULTITHREAD_SUPPORT
#include <omp.h>
const int MAX_THREADS = 64;
#endif


using namespace CSA;


int main(int argc, char** argv)
{
  std::cout << "RLCSA display test" << std::endl;
  if(argc < 4)
  {
    std::cout << "Usage: display_test basename sequences max_length [threads [random_seed]]" << std::endl;
    return 1;
  }

  std::cout << "Base name: " << argv[1] << std::endl;

  sint sequences = std::max(atoi(argv[2]), 1);
  std::cout << "Sequences: " << sequences << std::endl;

  usint max_length = std::max(atoi(argv[3]), 1);
  std::cout << "Prefix length: " << max_length << std::endl;

  sint threads = 1;
#ifdef MULTITHREAD_SUPPORT
  if(argc > 4)
  {
    threads = std::min(MAX_THREADS, std::max(atoi(argv[4]), 1));
  }
#endif
  std::cout << "Threads: " << threads << std::endl; 

  usint seed = 0xDEADBEEF;
  if(argc > 5)
  {
    seed = atoi(argv[5]);
  }
  std::cout << "Random seed: " << seed << std::endl; 
  std::cout << std::endl;

  RLCSA rlcsa(argv[1]);
  if(!rlcsa.supportsDisplay())
  {
    std::cerr << "Error: Display is not supported!" << std::endl;
    return 2;
  }
  rlcsa.printInfo();
  rlcsa.reportSize(true);

  usint total = 0;
  usint seq_num, seq_total = rlcsa.getNumberOfSequences();
  sint i;

  double start = readTimer();
  srand(seed);
  uchar* buffer = new uchar[max_length];
  #ifdef MULTITHREAD_SUPPORT
  usint length;
  usint thread_id;
  RLCSA* indexes[threads]; indexes[0] = &rlcsa;
  uchar* buffers[threads]; buffers[0] = buffer;
  for(i = 1; i < threads; i++)
  {
    indexes[i] = new RLCSA(&rlcsa);
    buffers[i] = new uchar[max_length];
  }
  omp_set_num_threads(threads);
  #pragma omp parallel private(seq_num, length, thread_id)
  {
    #pragma omp for schedule(dynamic, 1)
    for(i = 0; i < sequences; i++)
    {
      #pragma omp critical
      {
        seq_num = rand() % seq_total;
      }
      thread_id = omp_get_thread_num();
      length = indexes[thread_id]->displayPrefix(seq_num, max_length, buffers[thread_id]);
      #pragma omp critical
      {
        total += length;
      }
    }
  }
  for(i = 1; i < threads; i++) { delete indexes[i]; delete[] buffers[i]; }
  #else
  for(i = 0; i < sequences; i++)
  {
    seq_num = rand() % seq_total;
    total += rlcsa.displayPrefix(seq_num, max_length, buffer);
  }
  #endif
  delete[] buffer;

  double time = readTimer() - start;
  double megabytes = total / (double)MEGABYTE;
  std::cout << megabytes << " megabytes in " << time << " seconds (" << (megabytes / time) << " MB/s)" << std::endl;
  std::cout << std::endl;

  return 0;
}
