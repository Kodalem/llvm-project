
#include "FCFGPostDom.h"
#include "Patmos.h"
#include "SinglePath/PatmosSPReduce.h"
#include "SinglePath/ConstantLoopDominatorAnalysis.h" // for "get_intersection"
#include "TargetInfo/PatmosTargetInfo.h"
#include "llvm/ADT/GraphTraits.h"
#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/IR/Dominators.h"

#include <deque>
#include <optional>

using namespace llvm;

MachineBasicBlock* FCFGPostDom::outermost_inner_loop_header(MachineLoop *inner) {
	assert(inner && "Inner loop was null");

	auto outer_depth = loop? loop->getLoopDepth():0;

	assert(inner->getLoopDepth() > (outer_depth));
	while(inner->getLoopDepth() != (outer_depth+1)) {
		inner = inner->getParentLoop();
	}
	assert(loop == inner->getParentLoop());
	return inner->getHeader();
}

std::optional<MachineBasicBlock*> FCFGPostDom::fcfg_predecessor(MachineBasicBlock *pred) {
	auto *pred_loop = LI.getLoopFor(pred);

	if(pred_loop == loop) {
		return pred;
	} else if(!loop || loop->contains(pred)) {
		/// Predecessor is in nested loop, use header of outermost inner loop
		return outermost_inner_loop_header(pred_loop);
	} else {
		/// Predecessor is in outer loop
		return std::nullopt;
	}
}

 std::optional<MachineBasicBlock*> FCFGPostDom::fcfg_successor(MachineBasicBlock *succ) {
	auto *succ_loop = LI.getLoopFor(succ);

	if(succ_loop == loop) {
		return succ;
	} else if(!loop || loop->contains(succ)) {
		/// Successor is in nested loop, use header of outermost inner loop
		return outermost_inner_loop_header(succ_loop);
	} else {
		/// Successor is in outer loop
		return std::nullopt;
	}
}

void FCFGPostDom::calculate(std::set<MachineBasicBlock*> roots) {
	assert(std::all_of(roots.begin(), roots.end(), [&](auto root){
		return
			// Root is a node in the loop
			LI.getLoopFor(root) == loop ||
			// Root is a header of an inner loop
			(LI.getLoopFor(root)->getParentLoop() == loop && LI.getLoopFor(root)->getHeader() == root);
	}) && "All roots not in the same loop");

	std::deque<MachineBasicBlock*> worklist;

	auto add_preds_to_worklist = [&](auto current){
		std::for_each(current->pred_begin(), current->pred_end(), [&](auto pred){
			auto fcfg_pred = fcfg_predecessor(pred);
			if(fcfg_pred) {
				worklist.push_back(*fcfg_pred);
			}
		});
	};

	// Initial post dominators: roots are only post dominated by themselves
	for(auto root: roots) {
		post_doms[root] = {root};
		add_preds_to_worklist(root);
	}

	while(!worklist.empty()) {
		auto current = worklist.front();
		worklist.pop_front();
		if(post_doms.count(current)) {
			// already did it
			continue;
		}

		std::set<MachineBasicBlock*> succs;
		if(LI.isLoopHeader(current) && (!loop || loop->getHeader() != current)) {
			// Current is header of inner loop
			auto currents_loop = LI.getLoopFor(current);
			assert(currents_loop);
			assert(currents_loop->getParentLoop() == loop);

			SmallVector<std::pair<MachineBasicBlock*, MachineBasicBlock*>> exits;
			currents_loop->getExitEdges(exits);
			for(auto edge: exits) {
				if(!loop || loop->contains(edge.second)) {
					succs.insert(edge.second);
				}
			}
		} else {
			std::for_each(current->succ_begin(), current->succ_end(), [&](auto succ){
				return succs.insert(*fcfg_successor(succ));
			});
		}

		auto all_succs_done = std::all_of(succs.begin(), succs.end(), [&](auto succ){
			return post_doms.count(succ);
		});
		if(all_succs_done) {
			auto doms = post_doms[*succs.begin()];
			std::for_each(succs.begin(), succs.end(), [&](auto succ){
				doms = get_intersection(doms, post_doms[succ]);
			});
			doms.insert(current);
			post_doms[current] = doms;
			add_preds_to_worklist(current);
		} else {
			// Wait for all successors to be done
			worklist.push_back(current);
		}
	}
}

FCFGPostDom::FCFGPostDom(MachineLoop *l, MachineLoopInfo &LI): loop(l), LI(LI){
	assert(loop);
	SmallVector<std::pair<MachineBasicBlock*, MachineBasicBlock*>> exits;
	loop->getExitEdges(exits);

	std::set<MachineBasicBlock*> roots;
	for(auto exit: exits) {
		auto source = exit.first;
		auto source_loop = LI.getLoopFor(source);
		if(source_loop != loop) {
			roots.insert(outermost_inner_loop_header(source_loop));
		} else {
			roots.insert(source);
		}
	}

	// add the latches to the roots
	SmallVector<MachineBasicBlock*> latches;
	loop->getLoopLatches(latches);
	for(auto latch: latches) {
		roots.insert(latch);
	}

	calculate(roots);
	for(auto inner: *loop) {
		inner_doms.push_back(FCFGPostDom(inner, LI));
	}
}

void FCFGPostDom::get_post_dominees(MachineBasicBlock *dominator, std::set<MachineBasicBlock*> &dominees) {
	for(auto entry: post_doms) {
		if(entry.second.count(dominator)){
			dominees.insert(entry.first);
		}
	}

	for(auto inner: inner_doms) {
		inner.get_post_dominees(dominator, dominees);
	}
}

FCFGPostDom::FCFGPostDom(MachineFunction &MF, MachineLoopInfo &LI) : loop(nullptr), LI(LI){
	auto is_end = [&](auto &block){return block.succ_size() == 0;};
	assert(std::count_if(MF.begin(), MF.end(), is_end) == 1 && "Found multiple end blocks");
	auto end = std::find_if(MF.begin(), MF.end(), is_end);

	calculate({&*end});
	for(auto inner: LI) {
		inner_doms.push_back(FCFGPostDom(inner, LI));
	}
}

void FCFGPostDom::print(raw_ostream &O, unsigned indent) {
	if(indent == 0) O << "Post Dominators:\n";
	for(auto entry: post_doms) {
		auto block = entry.first;
		auto dominees = entry.second;

		for(int i = 0; i<indent; i++) O << "\t";
		O << "bb." << block->getNumber() << ": [";
		for(auto dominee: dominees) {
			O << "bb." << dominee->getNumber() << ", ";
		}
		O << "]\n";
	}
	for(auto inner: inner_doms) {
		inner.print(O, indent+1);
	}
}

bool FCFGPostDom::post_dominates(MachineBasicBlock *dominator, MachineBasicBlock *dominee) {
	return post_doms[dominee].count(dominator) ||
		std::any_of(inner_doms.begin(), inner_doms.end(),
			[&](auto inner){ return inner.post_dominates(dominator, dominee);});
}

void FCFGPostDom::get_control_dependencies(
	std::map<
		// X
		MachineBasicBlock*,
		// Set of {Y->Z} control dependencies of X
		std::set<std::pair< std::optional<MachineBasicBlock*>,MachineBasicBlock*>>
	> &deps
) {
	for(auto entry: post_doms) {
		auto block = entry.first;

		if(LI.isLoopHeader(block)) {
			// headers can only be dependent on the loop entry
			deps[block].insert(std::make_pair(std::nullopt, block));
		} else {
			std::set<MachineBasicBlock*> dominees;
			get_post_dominees(block, dominees);
			// x is control dependent on (y->z) if x post-doms z but not y.
			// 'block' is control dependent on ('pred'->'dominee') if 'block' post-doms 'dominee' but not 'pred'.
			for(auto dominee: dominees){
				// 'block' post-doms 'dominee'
				auto dominee_loop = LI.getLoopFor(dominee);

					if(dominee->pred_size() == 0 || (loop && loop->getHeader() == dominee)) {
					// dominee is the entry to the loop (header).
					// If you post dominate the entry, you are control dependent on the entry edge
						deps[block].insert(std::make_pair(std::nullopt, dominee));
				} else {
					std::for_each(dominee->pred_begin(), dominee->pred_end(), [&](auto pred){
						auto fcfg_pred = fcfg_predecessor(pred);
						assert(fcfg_pred && "Predecessor is header or entry");

						if (!post_dominates(block, *fcfg_pred)) {
							auto fcfg_predecessor_loop = LI.getLoopFor(*fcfg_pred);

							if(fcfg_predecessor_loop != loop) {
								// the predecessor is a loop header. Use the loop's exit edges as the dep edge
								SmallVector<std::pair<MachineBasicBlock*, MachineBasicBlock*>> exits;
								fcfg_predecessor_loop->getExitEdges(exits);

								for(auto exit: exits) {
									if(exit.second == dominee) {
										deps[block].insert(std::make_pair(std::optional<MachineBasicBlock*>{exit.first}, dominee));
									}
								}
							} else {
								assert(fcfg_pred);
								deps[block].insert(std::make_pair(fcfg_pred, dominee));
							}
						}
					});
				}
			}
		}
	}
	for(auto inner: inner_doms) {
		inner.get_control_dependencies(deps);
	}

	assert(
		std::all_of(deps.begin(), deps.end(), [&](auto entry){
			return std::all_of(entry.second.begin(), entry.second.end(), [&](auto edge){
				if(edge.first) {
					// Edge exists
					return std::any_of((*edge.first)->succ_begin(), (*edge.first)->succ_end(), [&](auto succ){
						return succ == edge.second;
					});
				} else {
					// Non-edges use only headers
					return LI.isLoopHeader(edge.second) || edge.second->pred_size() == 0;
				}
			});
		})
		&& "Not all dependencies are valid"
	);
}







