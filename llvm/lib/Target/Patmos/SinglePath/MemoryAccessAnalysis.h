#ifndef TARGET_PATMOS_SINGLEPATH_MEMORYACCESSANALYSIS_H_
#define TARGET_PATMOS_SINGLEPATH_MEMORYACCESSANALYSIS_H_

#include "llvm/CodeGen/MachineBasicBlock.h"
#include <map>
#include "queue"
#include <tuple>

#define DEBUG_TYPE "patmos-const-exec"

// Uncomment to always print debug info
// #define LLVM_DEBUG(...) __VA_ARGS__

namespace llvm {

template<
  typename MachineBasicBlock
>
using LoopCounts =
  // Access characteristics for each loop distinguished by its header block.
  std::map<
    // The header
    const MachineBasicBlock*,
    std::tuple<
      // Min/Max accesses for any path leading to the loop header
      std::pair<unsigned,unsigned>,
      // Min/Max accesses for any path in the loop starting from the loop header
      // until a latch
      std::pair<unsigned,unsigned>,
      // Min/Max accesses for any path starting in the parent of this loop,
      // entering the loop, and eventually exiting through the given block
      std::map<
        const MachineBasicBlock*,
        std::pair<unsigned,unsigned>
      >
    >
  >;

/// Gets the access count of this block within its innermost loop.
/// If no count is set, returns 0.
template<
  typename MachineBasicBlock
>
std::pair<unsigned,unsigned> getBlockCount(
  const MachineBasicBlock* from,
  std::map<const MachineBasicBlock*,std::pair<unsigned,unsigned>> &block_counts
){
  return block_counts.count(from) ? block_counts[from] : std::make_pair((unsigned)0,(unsigned)0);
}

// Gets memory access count of the block while taking into account
// if it is an exit edge of a loop deeper than the given depth.
template<
  typename MachineBasicBlock,
  typename MachineLoopInfo
>
std::pair<unsigned,unsigned> getExitCount(
    const MachineLoopInfo *LI,
    std::map<const MachineBasicBlock*, std::pair<unsigned,unsigned>> &block_counts,
    LoopCounts<MachineBasicBlock> &loop_counts,
    const MachineBasicBlock* from,
    unsigned depth
){
  assert(LI->getLoopDepth(from) >= depth && "Edge into middle of loop");
  if(LI->getLoopDepth(from) != depth) {
    // This is an exit edge from nested loop. Find the outermost loop
    auto *loop = LI->getLoopFor(from);
    while(loop->getLoopDepth() > (depth+1)) {
      loop = loop->getParentLoop();
      assert(loop && "Couldn't find shallower loop");
    }
    auto *header = loop->getHeader();

    return std::get<2>(loop_counts[header])[from];
  } else {
    return getBlockCount(from, block_counts);
  }
}

template<
  typename MachineBasicBlock,
  typename MachineLoopInfo,
  typename Iter,
  typename Filter,
  typename GetDepth
>
std::pair<unsigned, unsigned> minMaxForAll(
    const MachineLoopInfo *LI,
    std::map<const MachineBasicBlock*, std::pair<unsigned,unsigned>> &block_counts,
    LoopCounts<MachineBasicBlock> &loop_counts,
    Iter begin, Iter end, Filter filter, GetDepth loop_depth
) {
  unsigned min =  INT_MAX;
  unsigned max = 0;
  bool none = true;
  std::for_each(begin, end, [&](auto bb){
    if(filter(bb)){
      auto exitCount = getExitCount(LI, block_counts, loop_counts, bb, loop_depth(bb));
      assert(exitCount.first <= INT_MAX);
      min = std::min(min, exitCount.first);
      max = std::max(max, exitCount.second);
      none=false;
    }
  });
  return std::make_pair(none? 0:min, max);
}

/// Performs Memory Access Analysis, i.e., counting how many memory accesses different paths
/// may perform.
/// Should be given the entry block to the function to be analyzed,
/// then the loop bound information for that function,
/// then a function that given a block, counts the number of memory-accessing instructions it has,
/// and lastly, a function that given a block, if its a header, returns the maximum loop iterations
/// or if not, returns -1
///
/// We template such that we can use mocked MBBs when testing it.
template<
  typename MachineBasicBlock,
  typename MachineLoopInfo
>
std::pair<
  // The minimum/maximum number of accesses on any path leading to the block
  // starting from the innermost loop header (including both the loop header and the block itself)
  std::map<const MachineBasicBlock*, std::pair<unsigned,unsigned>>,
  LoopCounts<MachineBasicBlock>
>
memoryAccessAnalysis(
    const MachineBasicBlock *start_mbb,
    const MachineLoopInfo *LI,
    unsigned (*countAccesses)(const MachineBasicBlock *mbb),
    std::pair<uint64_t, uint64_t> (*loopBounds)(const MachineBasicBlock *mbb)
) {
  LLVM_DEBUG(dbgs() << "\nStarting Memory Access Analysis\n");

  std::map<const MachineBasicBlock*, std::pair<unsigned,unsigned>> block_counts;
  LoopCounts<MachineBasicBlock> loop_counts;
  std::queue<const MachineBasicBlock*> worklist;

  worklist.push(start_mbb);

  while(!worklist.empty()) {
    auto *current = worklist.front();
    worklist.pop();

    auto *loop = LI->getLoopFor(current);

    LLVM_DEBUG(
      dbgs() << "\nRecalculating block '" << current->getName() << "'\n";
    );

    if(LI->isLoopHeader(current)) {
      LLVM_DEBUG(
        dbgs() << "Is loop header.\n";
      );

      // Calculate count for block itself
      // Loop header restarts the count as if they have no predecessors
      auto new_count = countAccesses(current);

      auto had_assignment = block_counts.count(current);
      block_counts[current] = std::make_pair(new_count,new_count);
      LLVM_DEBUG(
        dbgs() << "Block '" << current->getName() << "' assigned: " << new_count << ", " << new_count << "\n";
      );
      // Enqueue successors if needed
      if(!had_assignment) {
        std::for_each(current->succ_begin(), current->succ_end(), [&](auto succ){
          if(succ != current) {
            LLVM_DEBUG(
              dbgs() << "Enqueueing '" << succ->getName() << "'\n";
            );
            worklist.push(succ);
          }
        });
      }

      // Now calculate characteristics for the loop

      // Find lowest/highest predecessor
      auto old_pred_counts = std::get<0>(loop_counts[current]);
      unsigned pred_min, pred_max;
      std::tie(pred_min, pred_max) = minMaxForAll(
          LI, block_counts, loop_counts,
          current->pred_begin(), current->pred_end(),
          [&](auto pred){return LI->getLoopDepth(pred) < loop->getLoopDepth();},
          [&](auto pred){return LI->getLoopDepth(pred);}
        );

      std::get<0>(loop_counts[current]) = std::make_pair(pred_min,pred_max);
      LLVM_DEBUG(
        if(pred_min < old_pred_counts.first || pred_max > old_pred_counts.second){
          dbgs() << "Assigned predecessor count: " << pred_min << ", " << pred_max << "\n";
        }
      );

      // Calculate min/max iteration count
      auto old_iter_count = std::get<1>(loop_counts[current]);

      SmallVector<MachineBasicBlock*> loop_ends;
//      loop->getExitingBlocks(loop_ends);
      loop->getLoopLatches(loop_ends);
      unsigned new_iter_min, new_iter_max;
      std::tie(new_iter_min, new_iter_max) = minMaxForAll(
          LI, block_counts, loop_counts,
          loop_ends.begin(), loop_ends.end(),
          [&](auto end){return true;},
          [&](auto end){return loop->getLoopDepth();}
        );

      std::get<1>(loop_counts[current]) = std::make_pair(new_iter_min,new_iter_max);
      LLVM_DEBUG(
        if(new_iter_min < old_iter_count.first || new_iter_max > old_iter_count.second){
          dbgs() << "Assigned single iteration count: " << new_iter_min << ", " << new_iter_max << "\n";
        }
      );

      // Calculate exit counts
      SmallVector<MachineBasicBlock*> loop_exits;
      loop->getExitingBlocks(loop_exits);
      for(auto *exit_block: loop_exits){
        auto old_exit_count = std::get<2>(loop_counts[current])[exit_block];
        auto exit_block_count = getExitCount(LI, block_counts, loop_counts, exit_block, loop->getLoopDepth());
        assert(loopBounds(loop->getHeader()).first >= 1 && "Shouldn't iterate less than once");
        auto min_full_loop_iters = loopBounds(loop->getHeader()).first - 1;
        auto max_full_loop_iters = loopBounds(loop->getHeader()).second - 1;

        unsigned new_min_exit_count =
            std::get<0>(loop_counts[current]).first +
            (std::get<1>(loop_counts[current]).first * min_full_loop_iters) +
            exit_block_count.first;
        unsigned new_max_exit_count =
            std::get<0>(loop_counts[current]).second +
            (std::get<1>(loop_counts[current]).second * max_full_loop_iters) +
            exit_block_count.second;

        std::get<2>(loop_counts[current])[exit_block] = std::make_pair(new_min_exit_count,new_max_exit_count);

        if(new_min_exit_count < old_exit_count.first || new_max_exit_count > old_exit_count.second) {
          LLVM_DEBUG(
            dbgs() << "Exit '" << exit_block->getName() << "' assigned: "
              << new_min_exit_count << ", " << new_max_exit_count << "\n";
          );
          std::for_each(exit_block->succ_begin(), exit_block->succ_end(), [&](auto succ){
            if(LI->getLoopDepth(succ) < loop->getLoopDepth()) {
              LLVM_DEBUG(
                dbgs() << "Enqueueing '" << succ->getName() << "' as loop exit\n";
              );
              worklist.push(succ);
            }
          });
        }
      }
    } else {
      auto old_count = getBlockCount(current, block_counts);

      // Get min/max of predecessors
      unsigned pred_min, pred_max;
      std::tie(pred_min, pred_max) = minMaxForAll(
          LI, block_counts, loop_counts,
          current->pred_begin(), current->pred_end(),
          [&](auto pred){return true;},
          [&](auto pred){return loop? loop->getLoopDepth():0;}
        );

      // Update count
      auto new_min_count = pred_min + countAccesses(current);
      auto new_max_count = pred_max + countAccesses(current);

      auto had_assignment = block_counts.count(current);
      block_counts[current] = std::make_pair(new_min_count,new_max_count);

      // Enqueue successors if needed
      if(new_min_count > old_count.first || new_max_count > old_count.second || !had_assignment) {
        LLVM_DEBUG(
          dbgs() << "Block '" << current->getName() << "' assigned: "
            << new_min_count << ", " << new_max_count << "\n";
        );
        std::for_each(current->succ_begin(), current->succ_end(), [&](auto succ){
          LLVM_DEBUG(
            dbgs() << "Enqueueing '" << succ->getName() << "'\n";
          );
          worklist.push(succ);
        });
      }
    }
  }
  return std::make_pair(block_counts, loop_counts);
}

template<
  typename MachineBasicBlock,
  typename MachineLoopInfo
>
std::map<
  std::pair<const MachineBasicBlock*, const MachineBasicBlock*>,
  unsigned
>
memoryAccessCompensation(
    const MachineBasicBlock *start_mbb,
    const MachineLoopInfo *LI,
    unsigned (*countAccesses)(const MachineBasicBlock *mbb)
){
  LLVM_DEBUG(dbgs() << "\nStarting Memory Access Compensation Analysis\n");

  std::map<
    std::pair<const MachineBasicBlock*, const MachineBasicBlock*>,
    unsigned
  > compensation;
  std::set<const MachineBasicBlock*> end_blocks;
  std::queue<std::tuple<
    const MachineBasicBlock*, // from
    const MachineBasicBlock*, // to
    unsigned                  // Missing count
  >> worklist;

  // First assign all edges their source's access counts
  std::for_each(start_mbb->succ_begin(), start_mbb->succ_end(), [&](auto succ){
    worklist.push(std::make_tuple(start_mbb, succ, 0));
  });
  while(!worklist.empty()) {
    const MachineBasicBlock *from, *to;
    unsigned count;
    std::tie(from,to,count) = worklist.front();
    worklist.pop();

    compensation[{from,to}] = countAccesses(from);

    std::for_each(to->succ_begin(), to->succ_end(), [&](auto succ){
      if(!(LI->isLoopHeader(to) && LI->getLoopFor(to)->contains(from))) {
        worklist.push(std::make_tuple(to, succ, countAccesses(to)));
      }
    });
  }
  worklist = {}; // Clear queue for reuse

  std::queue<const MachineBasicBlock*> header_worklist;
  std::set<const MachineBasicBlock*> done_headers;
  std::set<std::pair<const MachineBasicBlock*,const MachineBasicBlock*>> done_edges;
  header_worklist.push(start_mbb);

  while(!header_worklist.empty()){
    auto current_header = header_worklist.front();
    header_worklist.pop();


    std::for_each(current_header->succ_begin(), current_header->succ_end(), [&](auto succ){
      worklist.push(std::make_tuple(current_header, succ, 0));
      LLVM_DEBUG(
        dbgs() << "Enqueueing header's outgoing edge '" << current_header->getName()
          << "' -> '" << succ->getName() << "': " << 0 << "\n";
      );
    });

    while(!worklist.empty()) {
      const MachineBasicBlock *from, *to;
      unsigned count;
      std::tie(from,to,count) = worklist.front();
      worklist.pop();

      LLVM_DEBUG(
        dbgs() << "\nRecalculating Edge '" << from->getName() << "' -> '" << to->getName() << "': " << count << "\n";
      );

      auto to_is_end_block = to->succ_begin() == to->succ_end();
      auto to_multi_non_latch_preds = std::count_if(to->pred_begin(), to->pred_end(), [&](auto pred){
                              return !(LI->isLoopHeader(to) && LI->getLoopFor(to)->contains(pred));
                            }) > 1;
      auto to_header = LI->isLoopHeader(to);
      auto is_latch_edge = to_header && LI->getLoopFor(to)->contains(from);
      auto to_has_multiple_preds = to->pred_size() > 1;
      auto from_has_multiple_preds = from->pred_size() > 1;
      bool update_always;

      // Record any blocks that return from the function
      // for use at the end
      if(to_is_end_block) {
        end_blocks.insert(to);
      }

      if(
          to_is_end_block ||
          to_multi_non_latch_preds ||
          is_latch_edge
      ){
        auto original_comp = compensation[{from, to}];
        LLVM_DEBUG(
          dbgs() << "Assigning '" << from->getName() << "' -> '" << to->getName() << "': "
            << original_comp << " + " << count << "\n";
        );
        compensation[{from, to}] += count;
        count = 0;
        update_always = false;
      } else if( to_header ) {
        // When the target is a loop header, pass the accesses count on to the loop exits
        auto edge_comp = compensation[{from, to}];
        auto *loop = LI->getLoopFor(to);
        SmallVector<std::pair<MachineBasicBlock*, MachineBasicBlock*>> loop_exit_edges;
        loop->getExitEdges(loop_exit_edges);
        for(auto edge: loop_exit_edges) {
          LLVM_DEBUG(
            dbgs() << "Enqueueing Exit Edge '" << edge.first->getName()
              << "' -> '" << edge.second->getName() << "': " <<  edge_comp + count << "\n";
          );
          worklist.push(std::make_tuple(edge.first, edge.second, count + edge_comp));
        }
        compensation[{from, to}] = 0;
        count = 0;
        update_always = false;

      } else {
        auto edge_comp = compensation[{from, to}];
        count += edge_comp;
        compensation[{from, to}] = 0;
        update_always = true;
      }

      if(!LI->isLoopHeader(to) && (update_always || !is_latch_edge)) {
        std::for_each(to->succ_begin(), to->succ_end(), [&](auto succ){
            LLVM_DEBUG(
              dbgs() << "Enqueueing Edge '" << to->getName()
                << "' -> '" << succ->getName() << "': " << count << "\n";
            );
            worklist.push(std::make_tuple(to, succ, count));
        });
      }

      if(LI->isLoopHeader(to) && !done_headers.count(to)) {
        header_worklist.push(to);
        LLVM_DEBUG(
          dbgs() << "Enqueueing header '" << to->getName() << "'\n";
        );
        done_headers.insert(to);
      }
    }
  }
  LLVM_DEBUG(dbgs() << "\n");

  // Lastly, since end blocks don't have successors, we must account
  // for their accesses in their incoming edges.
  // So, add each end block's count to all its incoming edges.
  for(auto eb: end_blocks) {
    std::for_each(eb->pred_begin(), eb->pred_end(), [&](auto pred){
      auto old_value = compensation.count({pred, eb})? compensation[{pred, eb}] : 0;
      auto add_value = countAccesses(eb);
      if((old_value + add_value) != 0){
        LLVM_DEBUG(
          dbgs() << "Correcting end Edge '" << pred->getName() << "' -> '" <<
            eb->getName() << "': " << old_value << " + " << add_value << "'\n";
        );
        compensation[{pred, eb}] = old_value + add_value;
      }
    });

    // Remove any zero-assigned edges
    // This makes it easier to test (don't have to check 0-edges) and debug
    while(true){
      auto found = std::find_if(compensation.begin(), compensation.end(),[&](auto entry){
        return entry.second == 0;
      });
      if(found != compensation.end()) {
        compensation.erase(found);
      } else {
        break;
      }
    }
  }
  return compensation;
}

}
#endif /* TARGET_PATMOS_SINGLEPATH_MEMORYACCESSANALYSIS_H_ */
