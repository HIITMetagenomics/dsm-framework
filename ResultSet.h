// Dynamic result set class
// Originally by Gonzalo Navarro

#ifndef _RESULTSET_H_
#define _RESULTSET_H_

#ifndef ulong
typedef unsigned long ulong;
#endif

class ResultSet {
private:
    ulong n, lgn;
    ulong *tree;
    ulong lg(ulong);

    ulong conv (ulong p, ulong n, ulong lgn);
    ulong unconv (ulong p, ulong n, ulong lgn);
    ulong clearRangeLeft (ulong p1, ulong n, ulong pos, ulong pot);
    ulong clearRangeRight (ulong p2, ulong n, ulong pos, ulong pot);
    ulong clearBoth (ulong n, ulong p1, ulong p2, ulong pos, ulong pot);

    ulong nextSmallest (ulong n, ulong pos, ulong pot);
    ulong nextLarger (ulong n, ulong p, ulong pos, ulong pot);

    void prnspace (ulong k);
public:
    // creates empty results data structure for n nodes numbered 0..n-1
    ResultSet(ulong n);
    ~ResultSet();

    // returns 0/1 telling whether result p is not/is present in R
    bool get(ulong p);

    // inserts result p into R
    void set(ulong p);

    // clears all results p1..p2 in R
    void clearRange (ulong p1, ulong p2);
    // clears all results
    void clear();

    // returns pos of next(p) in R, or -1 if none
    ulong nextResult (ulong p);

};

#endif // _RESULTSET_H_
