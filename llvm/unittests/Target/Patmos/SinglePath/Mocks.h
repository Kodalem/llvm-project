#ifndef UNITTESTS_TARGET_PATMOS_SINGLEPATH_MOCKS_H_
#define UNITTESTS_TARGET_PATMOS_SINGLEPATH_MOCKS_H_

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <algorithm>

template<
  typename Payload
>
class MockMBB {
public:

  MockMBB(char* name, Payload p):
    payload(p), name(name),
      preds(nullptr), succs(nullptr)
  {}

  char* name;
  Payload payload;

  // We use pointers to vectors because using vectors directly always
  // resulted in segmentation fault. Don't know why. This is a workaround.
  std::vector<MockMBB<Payload>*> *preds;
  std::vector<MockMBB<Payload>*> *succs;

  void set_preds(std::vector<MockMBB<Payload>*> *new_preds){
    preds = new_preds;
  }

  void set_succs(std::vector<MockMBB<Payload>*> *new_succs){
      succs = new_succs;
  }

  typename std::vector<MockMBB<Payload>*>::const_iterator pred_begin() const {
      return preds->begin();
  }

  typename std::vector<MockMBB<Payload>*>::const_iterator pred_end() const {
      return preds->end();
  }

  unsigned pred_size() const {
      return preds->size();
  }

  typename std::vector<MockMBB<Payload>*>::const_iterator succ_begin() const {
      return succs->begin();
  }

  typename std::vector<MockMBB<Payload>*>::const_iterator succ_end() const {
      return succs->end();
  }

  unsigned succ_size() const {
      return succs->size();
  }

  std::string getName() const {
      return name;
  }
};

template<
  typename Payload
>
class MockLoop {
public:

  const MockLoop<Payload> *parent;
  std::vector<MockMBB<Payload>*> mbbs;
  std::vector<MockMBB<Payload>*> latches;
  std::vector<MockMBB<Payload>*> exits;

  MockLoop( const MockLoop<Payload> *parent,
      std::vector<MockMBB<Payload>*> new_mbbs,
      std::vector<MockMBB<Payload>*> new_latches,
      std::vector<MockMBB<Payload>*> new_exits
  ): parent(parent), mbbs(new_mbbs), latches(new_latches), exits(new_exits)
  {}

  MockMBB<Payload>* getHeader() const {
      return mbbs[0];
  }

  void getExitingBlocks(llvm::SmallVectorImpl<MockMBB<Payload> *> &out) const {
      out.append(exits.begin(),exits.end());
  }

  void getExitEdges(llvm::SmallVectorImpl<std::pair<MockMBB<Payload> *,MockMBB<Payload> *>> &out) const {
    llvm::SmallVector<MockMBB<Payload>*> loop_exits;
      getExitingBlocks(loop_exits);
      for(auto *exit_block: loop_exits){
        std::for_each(exit_block->succ_begin(), exit_block->succ_end(), [&](auto succ){
          if(!contains(succ)) {
            out.push_back(std::make_pair(exit_block, succ));
          }
        });
      }
  }

  void getLoopLatches(llvm::SmallVectorImpl<MockMBB<Payload> *> &out) const {
      out.append(latches.begin(),latches.end());
  }

  bool contains(const MockMBB<Payload> *mbb) const {
      return std::find(mbbs.begin(),mbbs.end(),mbb) != mbbs.end();
  }

  bool isLoopLatch(const MockMBB<Payload> *mbb) const {
      assert(contains(mbb) && "isLoopLatch must only be called on blocks that are part of the loop");
      return std::find(latches.begin(),latches.end(),mbb) != latches.end();
  }

  bool isLoopExiting(const MockMBB<Payload> *mbb) const {
      assert(contains(mbb) && "isLoopExiting must only be called on blocks that are part of the loop");
      return std::find(exits.begin(),exits.end(),mbb) != exits.end();
  }

  unsigned getLoopDepth() const {
      if(!parent){
        return 1;
      } else {
        return parent->getLoopDepth() + 1;
      }
  }

  const MockLoop<Payload>* getParentLoop() const {
    return parent;
  }

  typename std::vector<MockMBB<Payload>*>::const_iterator block_begin() const {
    return mbbs.begin();
  }

  typename std::vector<MockMBB<Payload>*>::const_iterator block_end() const {
    return mbbs.end();
  }
};

template<
  typename Payload
>
class MockLoopInfo {
public:

  std::vector<MockLoop<Payload>*> loops;
  MockLoopInfo()
  {}

  void add_loop(MockLoop<Payload>* l) {
      loops.push_back(l);
  }

  const MockLoop<Payload>* getLoopFor(const MockMBB<Payload> *mbb) const {
      unsigned depth = 0;
      const MockLoop<Payload>* result = nullptr;
      while(true) {
        auto found = std::find_if(loops.begin(), loops.end(),
            [&](auto l){ return l->contains(mbb) && l->getLoopDepth() > depth;});
        if(found != loops.end()) {
          result = *found;
          depth = result->getLoopDepth();
        } else {
            break;
        }
      }
      return result;
  }

  bool isLoopHeader(const MockMBB<Payload> *mbb) const {
      auto found = std::find_if(loops.begin(), loops.end(),
          [&](auto l){ return l->getHeader() == mbb;});
      return found != loops.end();
  }

  unsigned getLoopDepth(const MockMBB<Payload> *mbb) const {
      auto loop = getLoopFor(mbb);
      if(loop) {
          return loop->getLoopDepth();
      } else {
          return 0;
      }
  }

  typename std::vector<MockLoop<Payload>*>::const_iterator begin() const{
    return loops.begin();
  }

  typename std::vector<MockLoop<Payload>*>::const_iterator end() const{
    return loops.end();
  }
};

/// Used to protect literal arrays from the preprocessor.
/// see https://stackoverflow.com/questions/5503362/passing-array-literal-as-macro-argument
#define arr(...) __VA_ARGS__

/// Easy way to define the predecessors and successors of a block.
/// Lists must be given in braces '{...}'
#define set_preds_succs(block, preds, succs) \
  std::vector<MockMBB*> block ## _pred = preds; \
  std::vector<MockMBB*> block ## _succ = succs; \
  block.set_preds(& block ## _pred); \
  block.set_succs(& block ## _succ);

#define mockMBBBase(name, payload) auto name = MockMBB(#name, payload)

#endif /* UNITTESTS_TARGET_PATMOS_SINGLEPATH_MOCKS_H_ */
