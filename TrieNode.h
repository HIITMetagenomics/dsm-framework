/**
 * Handle trie 
 *
 */

#ifndef _TrieNode_H_
#define _TrieNode_H_

//#include "TrieReader.h"

#include <string>
#include <vector>
#include <map>
#include <cstdlib> // exit()
#include <cassert>

class TrieReader;

/**
 * Base class to read input
 */
/*class TrieNode
{
public:
    // Construct root node
    explicit TrieNode(std::vector<TrieReader *> tr)
        : treaders(tr), root(true), symbol(0), level(0), parent(0)
    { }

    TrieNode(char c, int lvl, TrieNode *prnt)
        : root(false), symbol(c), level(lvl), parent(prnt)
    { }

    TrieNode *updateReaders();


    TrieNode* getParent()
    { return parent; }
    TrieNode* getChild(char c)
    {
        std::map<char,TrieNode*>::iterator it = children.find(c);
        if (it != children.end())
            return it->second;
        
        // Create new child
        TrieNode *newChild  = new TrieNode(c, level+1, this);
        children[c] = newChild;
        return newChild;
    }
    bool isRoot()
    { return root; }
    int getLevel()
    { return level; }
    char getSymbol()
    { return symbol; }
    std::string path()
    {
        if (root)
            return std::string();
        return parent->path() + symbol;
    }
    int size()
    {
        int s = 1;
        for (std::map<char,TrieNode *>::iterator it = children.begin(); it != children.end(); ++it)
            s += it->second->size();
        return s;
    }


    ~TrieNode() 
    {
        // Note: treaders are not free'd here!
        
        for  (std::map<char,TrieNode *>::iterator it = children.begin(); it != children.end(); ++it)
            delete it->second;
    }

    static void setDebug(bool d)
    { debug = d; }
    static void setMaxReaders(int i)
    { maxreaders = i; }
protected:

    std::vector<TrieReader *> treaders;
    bool root;
    char symbol;
    int level;

    TrieNode *parent;

    std::map<char,TrieNode *> children;

    static bool debug;
private:
    TrieNode();
    // No copy constructor and assignment
    TrieNode(TrieNode const&);
    TrieNode& operator = (TrieNode const&);

    ulong n;    
};*/

#endif // _TrieNode_H_
