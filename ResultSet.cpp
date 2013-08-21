
#include <iostream> 
#include "ResultSet.h"



#define W (8*sizeof(ulong))

#if __WORDSIZE == 64
#define logW 6lu // 64bit system, code checks = lg(W)-1
#else
#define logW 5lu // 32bit system, code checks = lg(W)-1
#endif

#define divW(p) ((p)>>logW)
#define modW(p) ((p)&(W-1))

#define setBit(arr,pos) ((arr)[divW(pos)] |= 1lu<<modW(pos))
#define clearBit(arr,pos) ((arr)[divW(pos)] &= ~(1lu<<modW(pos)))
#define getBit(arr,pos) ((arr)[divW(pos)] & (1lu<<modW(pos))) // 0 or !=0


ulong ResultSet::lg (ulong n)
{ 
    ulong answ=0;
    while (n) { n>>=1lu; answ++; }
    return answ;
}

ResultSet::ResultSet(ulong n)
{ 
    if (logW != lg(W)-1)
    { 
        std::cerr << "Error, redefine logW as " << lg(W)-1 << " and recompile\n";
//        exit(1);
    }
    this->n = 2*n-1;
    this->lgn = lg(n);
    this->tree = new ulong[(this->n+W-1)/W];
    if (!this->tree)
    {
        std::cerr << "Malloc failed at ResultSet class." << std::endl;
        //      exit(1);
    }
    clearBit(this->tree,0); // clear all
}

ResultSet::~ResultSet()
{ 
    delete [] tree;
}

// to map 1..n to the bottom of the tree when n is not a power of 2
ulong ResultSet::conv (ulong p, ulong n, ulong lgn)

  { ulong t = n+1-(1lu<<lgn);
    if (p < t) return p;
    return (p<<1lu)-t;
  }

ulong ResultSet::unconv (ulong p, ulong n, ulong lgn)

  { ulong t = n+1-(1lu<<lgn);
    if (p < t) return p;
    return (p+t)>>1lu;
  }

bool ResultSet::get (ulong p) // returns 0 or 1

  { 
    ulong pos = 0;
    ulong pot = this->lgn;
    p = conv(p,n,pot);
    do { if (!getBit(this->tree,pos)) return false;
	  pot--;
	  pos = (pos<<1lu)+1+(p>>pot);
	  p &= ~(1lu<<pot);
	}
    while (pos < n);
    return true;
  }

void ResultSet::set (ulong p)

  { 
    ulong pos = 0;
    ulong npos;
    ulong pot = this->lgn;
    p = conv(p,n,pot);
    do { npos = (pos<<1lu)+1;
	 if (!getBit(this->tree,pos))
	    { setBit(this->tree,pos);
	      if (npos < n) 
		 { clearBit(this->tree,npos);
	           clearBit(this->tree,npos+1);
		 }
	    }
	  pot--;
	  pos = npos+(p>>pot);
	  p &= ~(1lu<<pot);
	}
    while (pos < n);
  }

	// returns final value of bit at pos
	
ulong ResultSet::clearRangeLeft (ulong p1, ulong n, ulong pos, ulong pot)
 { ulong npos;
    ulong bit;
    if (!getBit(tree,pos)) return 0; // range already zeroed
    p1 &= ~(1lu<<pot);
    if (p1 == 0) // full range to clear
       { clearBit(tree,pos); return 0; }
	// p1 != 0, there must be children
    pot--;
    npos = (pos<<1lu)+1;
    if ((p1>>pot) == 0) // go left, clear right
       { clearBit(tree,npos+1);
	 bit = clearRangeLeft(p1,n,npos,pot);
       } 
    else // go right
       { bit = clearRangeLeft(p1,n,npos+1,pot);
	 if (!bit) bit = getBit(tree,npos);
       }
    if (!bit) clearBit(tree,pos);
    return bit;
  }

ulong ResultSet::clearRangeRight (ulong p2, ulong n, ulong pos, ulong pot)

  { ulong npos;
    ulong bit;
    if (!getBit(tree,pos)) return 0; // range already zeroed
    p2 &= ~(1lu<<pot);
    if (p2 == 0) return 1; // empty range to clear, and bit is 1 for sure
	// p2 != 0, there must be children
    pot--;
    npos = (pos<<1lu)+1;
    if ((p2>>pot) == 1) // go right, clear left
       { clearBit(tree,npos);
	 bit = clearRangeRight(p2,n,npos+1,pot);
       } 
    else // go left
       { bit = clearRangeRight(p2,n,npos,pot);
	 if (!bit) bit = getBit(tree,npos+1);
       }
    if (!bit) clearBit(tree,pos);
    return bit;
  }

ulong ResultSet::clearBoth (ulong n, ulong p1, ulong p2, ulong pos, ulong pot)

  { ulong npos,npos1,npos2;
    ulong bit;
    if (!getBit(tree,pos)) return 0; // range is already zeroed
    npos = (pos<<1lu)+1;
	// children must exist while the path is unique, as p1<p2
    pot--;
    npos1 = npos+(p1>>pot);
    npos2 = npos+(p2>>pot);
    if (npos1 == npos2) // we're inside npos1=npos2
       { bit = clearBoth (n,p1&~(1lu<<pot),p2&~(1lu<<pot),npos1,pot);
	 bit |= getBit(tree,npos+1-(p1>>pot)); // the other
       }
     else  // p1 and p2 take different paths here
	{ bit  = clearRangeLeft(p1,n,npos1,pot); 
          bit |= clearRangeRight(p2,n,npos2,pot);
	}
     if (!bit) clearBit(tree,pos);
     return bit;
  }

void ResultSet::clearRange (ulong p1, ulong p2)

  { if ((p2+1)<<1lu > this->n) 
         clearRangeLeft(conv(p1,this->n,this->lgn),this->n,0,this->lgn);
    else clearBoth(n,conv(p1,n,this->lgn),conv(p2+1,n,lgn),0,lgn);
  }

ulong ResultSet::nextSmallest (ulong n, ulong pos, ulong pot)

  { ulong p = 0;
    while (1)
       { pot--;
	 pos = (pos<<1lu)+1;
	 if (pos >= n) return p;
	 if (!getBit(tree,pos)) { pos++; p |= (1lu<<pot); }
       }
  }

ulong ResultSet::nextLarger (ulong n, ulong p, ulong pos, ulong pot)

  { ulong answ;
    if (!getBit(tree,pos)) return -1; // no answer
    pot--;
    pos = (pos<<1lu)+1;
    if (pos >= n) return 0; // when n is not a power of 2, missing leaves
    if ((p>>pot) == 0) // p goes left
       { answ = nextLarger(n,p&~(1lu<<pot),pos,pot);
	 if (answ != ~0lu) return answ;
         if (!getBit(tree,pos+1)) return -1; // no answer
	 return (1lu<<pot) | nextSmallest(n,pos+1,pot);
       }
    else 
       { answ = nextLarger(n,p&~(1lu<<pot),pos+1,pot);
         if (answ != ~0lu) return (1lu<<pot) | answ;
         return ~0lu;
       }
  }


void  ResultSet::clear()
{
    clearBit(this->tree,0);
}

ulong ResultSet::nextResult (ulong p) // returns pos of next(p) or -1 if none

  { ulong answ;
    if (((p+1)<<1lu) > this->n) return ~0lu; // next(last), p+1 out of bounds
    answ = nextLarger(this->n,conv(p+1,this->n,this->lgn),0,this->lgn);
    if (answ == ~0u) return ~0u;
    return unconv(answ,this->n,this->lgn);
  }


