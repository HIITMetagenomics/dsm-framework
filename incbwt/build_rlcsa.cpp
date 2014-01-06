#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>

#include "rlcsa_builder.h"
#include "misc/utils.h"


using namespace CSA;



int
lineByLineRLCSA(std::string base_name, usint block_size, usint sample_rate, usint buffer_size, bool lower_case)
{
  Parameters parameters;
  parameters.set(RLCSA_BLOCK_SIZE.first, block_size);
  parameters.set(SAMPLE_RATE.first, sample_rate);

  if(sample_rate > 0)
  {
    parameters.set(SUPPORT_LOCATE.first, 1);
    parameters.set(SUPPORT_DISPLAY.first, 1);
  }
  else
  {
    parameters.set(SUPPORT_LOCATE.first, 0);
    parameters.set(SUPPORT_DISPLAY.first, 0);
  }
  parameters.print();

  std::cout << "Input: " << base_name << std::endl;
  std::ifstream input_file(base_name.c_str(), std::ios_base::binary);
  if(!input_file)
  {
    std::cerr << "Error opening input file!" << std::endl;
    return 2;
  }
  std::cout << "Buffer size: " << buffer_size << " MB" << std::endl;
  if(lower_case) { std::cout << "Converting to lower case." << std::endl; }
  std::cout << std::endl;

  double start = readTimer();
  RLCSABuilder builder(parameters.get(RLCSA_BLOCK_SIZE), parameters.get(SAMPLE_RATE), buffer_size * MEGABYTE);

  usint lines = 0, total = 0;
  while(input_file)
  {
    char buffer[16384];  // FIXME What if lines are longer? Probably fails.
    input_file.getline(buffer, 16384);
    usint chars = input_file.gcount();
    lines++; total += chars;
    if(lower_case && chars > 0) { for(usint i = 0; i < chars - 1; i++) { buffer[i] = tolower(buffer[i]); } }
    if(chars >= 16383) { std::cout << lines << ": " << chars << " chars read!" << std::endl; }
    if(chars > 1) { builder.insertSequence(buffer, chars - 1, false); }
  }

  RLCSA* rlcsa = 0;
  if(builder.isOk())
  {
    rlcsa = builder.getRLCSA();
    rlcsa->writeTo(base_name);
  }
  else
  {
    std::cerr << "Error: RLCSA construction failed!" << std::endl;
    return 3;
  }

  double time = readTimer() - start;
  double build_time = builder.getBuildTime();
  double search_time = builder.getSearchTime();
  double sort_time = builder.getSortTime();
  double merge_time = builder.getMergeTime();

  double megabytes = rlcsa->getSize() / (double)MEGABYTE;
  usint sequences = rlcsa->getNumberOfSequences();
  std::cout << sequences << " sequences" << std::endl;
  std::cout << megabytes << " megabytes in " << time << " seconds (" << (megabytes / time) << " MB/s)" << std::endl;
  std::cout << "(build " << build_time << " s, search " << search_time << "s, sort " << sort_time << " s, merge " << merge_time << " s)" << std::endl;
  std::cout << std::endl;

  delete rlcsa;
  return 0;
}


int
main(int argc, char** argv)
{
  std::cout << "Line-by-line RLCSA builder" << std::endl;

  char* base_name = 0;
  usint buffer_size = 0, block_size = 0, sample_rate = 0;
  bool lower_case = false;
  int name_arg = 1, buffer_arg = 2, block_arg = 3, sample_arg = 4;
  if(argc > 1 && argv[1][0] == '-' && argv[1][1] == 'l')
  {
    lower_case = true;
    name_arg++; buffer_arg++; block_arg++; sample_arg++;
  }
  if(argc > name_arg) { base_name = argv[name_arg]; }
  if(argc > buffer_arg) { buffer_size = atoi(argv[buffer_arg]); }
  if(argc > block_arg) { block_size = atoi(argv[block_arg]); }
  if(argc > sample_arg) { sample_rate = atoi(argv[sample_arg]); }

  if(buffer_size == 0)
  {
    std::cout << "Usage: build_rlcsa [-l] base_name buffer_size [block_size [sample_rate]]" << std::endl;
    std::cout << "  -l  Convert to lower case." << std::endl;
    return 1;
  }
  std::cout << std::endl;

  return lineByLineRLCSA(base_name, block_size, sample_rate, buffer_size, lower_case);
}
