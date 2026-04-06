
#ifndef TARGET_PATMOS_SINGLEPATH_FCFGPOSTDOM_H_
#define TARGET_PATMOS_SINGLEPATH_FCFGPOSTDOM_H_

#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineLoopInfo.h"

#include <set>
#include <optional>
#include <map>
#include <vector>
#include <utility>

#include "llvm/Support/raw_ostream.h"

namespace llvm {

/// Calculate the postdominators for the forward CFG (FCFG) of a function.
class FCFGPostDom {
private:
	std::map<
		MachineBasicBlock*,
		// Blocks that post dominate this block
		std::set<MachineBasicBlock*>
	> post_doms;

	/// The loop of the FCFG
	MachineLoop *loop;

	/// The loopinfo of the funtion
	MachineLoopInfo &LI;

	/// FCFG doms of inner loops
	std::vector<FCFGPostDom> inner_doms;

	/// Calculate the post dominators for this FCFG. Does not calculate for inner loops.
	void calculate(std::set<MachineBasicBlock*> roots);

	/// Returns (through 'dominees') the blocks that the given block post dominates in the FCFG.
	/// This looks at inner loops too.
	void get_post_dominees(MachineBasicBlock *dominator, std::set<MachineBasicBlock*> &dominees);

public:

	/// Create the FCFG post dominators for the given loop.
	///
	/// Cannot be used for the outer-most pseudo-loop (where loop == NULL)
	/// for that, use the other constructor
	FCFGPostDom(MachineLoop *l, MachineLoopInfo &LI);

	/// Create the FCFG post dominators for the function
	FCFGPostDom(MachineFunction &MF, MachineLoopInfo &LI);

	void print(raw_ostream &O, unsigned indent = 0);

	/// Returns whether the first block post dominates the second in the FCFG or any inner FCFGs
	bool post_dominates(MachineBasicBlock *dominator, MachineBasicBlock *dominee);

	/// Calculate control dependencies.
	/// X is control dependent on Y if X post-dominates Z but not Y (given the edge Y->Z).
	///
	/// This looks at all FCFGs
	void get_control_dependencies(std::map<
			// X
			MachineBasicBlock*,
			// Set of {Y->Z} control dependencies of X
			std::set<std::pair<std::optional<MachineBasicBlock*>,MachineBasicBlock*>>
		> &Deps);

	/// Returns the header of the outermost loop, containing the given loop,
	/// within the current loop.
	MachineBasicBlock* outermost_inner_loop_header(MachineLoop *inner);

	/// Given a predecessor block in the raw CFG, returns the equivalent predecessor in the
	/// FCFG (if any). Assumes the source block is in this loop.
	std::optional<MachineBasicBlock*> fcfg_predecessor(MachineBasicBlock *pred);

	/// Given a successor block in the raw CFG, returns the equivalent successor in the
	/// FCFG (if any). Assumes the source block is in this loop.
	std::optional<MachineBasicBlock*> fcfg_successor(MachineBasicBlock *succ);

};
}

#endif /* TARGET_PATMOS_SINGLEPATH_FCFGPOSTDOM_H_ */
