#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>

#include "rlcsa.h"
#include "misc/utils.h"


using namespace CSA;


int main(int argc, char** argv)
{
  std::cout << "Markov chain text generator" << std::endl;
  if(argc < 5)
  {
    std::cout << "Usage: text_generator index_name initial_context context_length output" << std::endl;
    return 1;
  }

  std::cout << "Index name: " << argv[1] << std::endl;
  std::cout << "Initial context: " << argv[2] << std::endl;

  usint context_length = atoi(argv[3]);
  std::cout << "Context length: " << context_length << std::endl;

  std::cout << "Output: " << argv[4] << std::endl;
  std::ofstream output(argv[4], std::ios_base::binary);
  if(!output)
  {
    std::cerr << "Error creating output file!" << std::endl;
    return 2;
  }
  std::cout << std::endl;

  RLCSA rlcsa(argv[1]);
  srand((long)readTimer());
  uchar* buffer = new uchar[MEGABYTE];
  usint len = rlcsa.generateText(argv[2], context_length, buffer, MEGABYTE);
  output.write((char*)buffer, len);

  delete[] buffer;
  output.close();
  return 0;
}
