#include "TextCollection.h"
#include "FMIndex.h"
//#include "rlcsa_wrapper.h"

#include <iostream>
#include <string>
#include <cstdio>
using std::string;


const string TextCollection::REVERSE_EXTENSION = ".reverse";
const string TextCollection::ROTATION_EXTENSION = ".rotation";

const string TextCollection::FMINDEX_EXTENSION = ".fmi";
const string TextCollection::RLCSA_EXTENSION = ".rlcsa.array";

bool fileexists(string const & filename)
{
    FILE *fp = std::fopen(filename.c_str(),"r");
    if(!fp)
        return false;

    std::fclose(fp);
    return true;
}

TextCollection * TextCollection::load(string const & filename, string const & samplefile)
{
  // Does filename determine index type to be FM-index?
  std::size_t found = filename.rfind(FMINDEX_EXTENSION);
  if(found != string::npos)
  {
      return new FMIndex(filename.substr(0, found), samplefile);
  }

  // Does filename determine index type to be RLCSA?
  found = filename.rfind(RLCSA_EXTENSION);
  if(found != string::npos)
  {
      std::cerr << "error: Currently unsupported" << std::endl;
      std::abort();
      //return new RLCSAWrapper(filename.substr(0, found));
  }

  // Does filename + RLCSA_EXTENSION exist?
  /*if(fileexists(filename + RLCSA_EXTENSION))
  {
    if(fileexists(filename + FMINDEX_EXTENSION))
    {
      std::cerr << "Warning: both indexes exist, using " << filename + RLCSA_EXTENSION << " by default." << std::endl;
    }
    return new RLCSAWrapper(filename);
    }*/

  // Does filename + FMINDEX_EXTENSION exist?
  if(fileexists(filename + FMINDEX_EXTENSION))
  {
      return new FMIndex(filename, samplefile);
  }

  return 0;
}

