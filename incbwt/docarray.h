#ifndef DOCARRAY_H
#define DOCARRAY_H

#include <stack>

#include "rlcsa.h"


namespace CSA
{

struct STNode
{
  uint      string_depth;
  pair_type range;
  STNode*   parent;
  STNode*   child;
  STNode*   sibling;
  STNode*   next;

  uint      stored_documents;
  uint      id;
  bool      contains_all;

  STNode(uint lcp, pair_type sa_range);
  ~STNode();

  void addChild(STNode* node);
  void addSibling(STNode* node);

  void deleteChildren();

  void addLeaves();   // Adds leaves to the sparse suffix tree when required.
  STNode* addLeaf(STNode* left, STNode* right, usint pos);

  bool verifyTree();  // Call this to ensure that the tree is correct.

  // Removes this node from the tree, replacing it by its children.
  void remove();

  void determineSize(uint& nodes, uint& leaves);
  void computeStoredDocuments();
  void setNext();

};

std::ostream& operator<<(std::ostream& stream, const STNode& data);

class DocArray
{
  public:
    DocArray(STNode* root, const RLCSA& _rlcsa);
    DocArray(const std::string& base_name, const RLCSA& _rlcsa, bool load_grammar = true);
    ~DocArray();

    void readRules(const std::string& name_prefix, bool print = false);

    void writeTo(const std::string& base_name) const;
    usint reportSize(bool print = false) const;

    std::vector<usint>* listDocuments(const std::string& pattern) const;
    std::vector<usint>* listDocuments(pair_type range) const;
    std::vector<usint>* directListing(pair_type range) const; // Use RLCSA directly.

    inline bool isOk() const { return this->ok; }
    inline bool hasGrammar() const { return this->has_grammar; }
    inline usint getSize() const { return this->rlcsa.getSize(); }

    inline usint getNumberOfNodes() const { return this->getNumberOfLeaves() + this->getNumberOfInternalNodes(); }
    inline usint getNumberOfLeaves() const { return this->leaf_ranges->getNumberOfItems(); }
    inline usint getNumberOfInternalNodes() const { return this->first_children->getNumberOfItems(); }

    inline usint maxInteger() const { return this->getNumberOfDocuments() + this->getNumberOfRules(); }
    inline usint getNumberOfDocuments() const { return this->rlcsa.getNumberOfSequences(); }
    inline usint getNumberOfRules() const { return this->rule_borders->getNumberOfItems(); }

//--------------------------------------------------------------------------

    /*
      In document graph, nodes 1 to ndoc are document ids and ndoc + 1 to ndoc + |nodes|
      are node/block ids.
    */

    inline bool nodeIsDoc(usint node_id) const
    {
      return (node_id > 0 && node_id <= this->getNumberOfDocuments());
    }

    inline bool nodeIsBlock(usint node_id) const
    {
      return (node_id > this->getNumberOfDocuments() &&
              node_id <= this->getNumberOfDocuments() + this->getNumberOfNodes());
    }

    inline usint nodeToDoc(usint node_id) const { return node_id - 1; }
    inline usint nodeToBlock(usint node_id) const { return node_id - this->getNumberOfDocuments() - 1; }

//--------------------------------------------------------------------------

  private:
    const static usint RANGE_BLOCK_SIZE = 32;
    const static usint PARENT_BLOCK_SIZE = 32;
    const static usint RULE_BLOCK_SIZE = 32;
    const static usint BLOCK_BLOCK_SIZE = 32;

    const RLCSA&    rlcsa;

    DeltaVector*    leaf_ranges;
    SuccinctVector* first_children;
    ReadBuffer*     parents;
    ReadBuffer*     next_leaves;  // this->getNumberOfLeaves() denotes that there is no next leaf.

    SuccinctVector* rule_borders;
    ReadBuffer*     rules;        // this->getNumberOfDocs() means all documents.

    SuccinctVector* block_borders;
    ReadBuffer*     blocks;       // Value this->maxInteger() means all documents.

    bool ok, has_grammar;

//--------------------------------------------------------------------------

    void processRange(pair_type range, std::vector<usint>& result) const;
    bool addBlocks(usint first, usint number, std::vector<usint>& result) const;
    void addRun(usint from, usint length, std::vector<usint>& result) const;
    std::vector<usint>* allDocuments() const;

    inline pair_type parentOf(usint tree_node, SuccinctVector::Iterator& iter) const
    {
      usint par = this->parents->readItem(iter.rank(tree_node) - 1);
      usint next_leaf = this->next_leaves->readItem(par);
      return pair_type(par + this->getNumberOfLeaves(), next_leaf);
    }

//--------------------------------------------------------------------------

    // These are not allowed.
    DocArray();
    DocArray(const DocArray&);
    DocArray& operator = (const DocArray&);
};


} // namespace CSA


#endif // DOCARRAY_H
