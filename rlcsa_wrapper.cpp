#include "rlcsa_wrapper.h"


RLCSAWrapper::RLCSAWrapper(const CSA::RLCSA* index) :
  rlcsa(index)
{
}

RLCSAWrapper::RLCSAWrapper(const std::string& base_name) :
  rlcsa(new CSA::RLCSA(base_name))
{
}

RLCSAWrapper::~RLCSAWrapper()
{
  delete this->rlcsa;
}

void
RLCSAWrapper::save(const std::string& base_name) const
{
   this->rlcsa->writeTo(base_name);
}

//--------------------------------------------------------------------------

TextCollection::TextPosition
RLCSAWrapper::getLength() const
{
  return this->rlcsa->getSize() + this->rlcsa->getNumberOfSequences();
}

TextCollection::TextPosition
RLCSAWrapper::getLength(DocId i) const
{
  return CSA::length(this->rlcsa->getSequenceRange(i));
}

TextCollection::TextPosition
RLCSAWrapper::LF(uchar c, TextPosition i) const
{
  if(i == (TextPosition)-1 || i < this->rlcsa->getNumberOfSequences()) 
  { return this->rlcsa->C(c); }
  return this->rlcsa->LF(i - this->rlcsa->getNumberOfSequences(), c);
}

uchar*
RLCSAWrapper::getSuffix(TextPosition pos, unsigned l) const
{
  uchar* text = new uchar[l + 1];

  if(l == 0 || pos < this->rlcsa->getNumberOfSequences())
  {
    text[0] = 0;
    return text;
  }
  pos -= this->rlcsa->getNumberOfSequences();

  unsigned n = this->rlcsa->displayFromPosition(pos, l, text);
  text[n] = 0;
  return text;
}

void
RLCSAWrapper::getPosition(position_result &result, TextPosition i) const
{
  if(i < this->rlcsa->getNumberOfSequences())
  {
    result.first = i;
    result.second = CSA::length(this->rlcsa->getSequenceRange(i));
    return;
  }

  result = convert(this->rlcsa->getRelativePosition(this->rlcsa->locate(i - this->rlcsa->getNumberOfSequences())));
}

void
RLCSAWrapper::getPosition(position_vector& results, TextPosition sp, TextPosition ep) const
{
// FIXME Assumes position_vector<unsigned,ulong>
    /*ep = std::min(ep, this->getLength() - 1);
  if(ep < sp) { return; }
  results.reserve(results.size() + ep + 1 - sp);

  while(sp < this->rlcsa->getNumberOfSequences())
  {
    results.push_back(position_result(sp, CSA::length(this->rlcsa->getSequenceRange(sp))));
    sp++;
  }

  sp -= this->rlcsa->getNumberOfSequences();
  ep -= this->rlcsa->getNumberOfSequences();
  CSA::usint* positions = this->rlcsa->locate(CSA::pair_type(sp, ep));
  if(positions == 0) { return; }
  for(TextPosition i = sp; i <= ep; i++)
  {
    results.push_back(convert(this->rlcsa->getRelativePosition(positions[i - sp])));
  }
  delete[] positions;*/
}
