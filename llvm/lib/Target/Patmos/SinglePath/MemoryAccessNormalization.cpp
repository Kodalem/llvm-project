//===-- MemoryAccessAnalysis.cpp - Remove unused function declarations ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Analyses single-path functions and decides which compensation algorithms
// to use for each.
//
// If the Decrementing Counter Compensation algorithm is chosen for a function,
// it is run immediately. If Opposite Predicate Compensation is chosen,
// a function attribute is set, such that a later pass can perform the compensation.
//
//===----------------------------------------------------------------------===//

#include "MemoryAccessNormalization.h"
#include "MemoryAccessAnalysis.h"
#include "OppositePredicateCompensation.h"
#include "PatmosSinglePathInfo.h"
#include "PatmosMachineFunctionInfo.h"
#include "TargetInfo/PatmosTargetInfo.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/CodeGen/MachineFrameInfo.h"

#include <deque>
#include <set>

using namespace llvm;

#define DEBUG_TYPE "patmos-const-exec"

STATISTIC(CounterComp, "Number of functions using 'counter' algorithm for constant execution time");
STATISTIC(OppositeComp, "Number of functions using 'opposite' algorithm for constant execution time");
STATISTIC(NoComp, "Number of functions not needing any compensation for constant execution time");
STATISTIC(CntCmpInstr, "Number of non-phi instructions added by the 'counter' compensation algorithm");

char MemoryAccessNormalization::ID = 0;

FunctionPass *llvm::createMemoryAccessNormalizationPass(const PatmosTargetMachine &tm) {
  return new MemoryAccessNormalization(tm);
}

/// returns the number of main-memory accessing instruction in the given block
static unsigned countAccesses(const MachineBasicBlock *mbb){
  auto count = 0;
  for(auto instr_iter = mbb->instr_begin(); instr_iter != mbb->instr_end(); instr_iter++){
    switch( instr_iter->getOpcode() ){
      case Patmos::LWM:
      case Patmos::LHM:
      case Patmos::LBM:
      case Patmos::LHUM:
      case Patmos::LBUM:
      case Patmos::SWM:
      case Patmos::SHM:
      case Patmos::SBM:
        count++;
        break;
      default:
        break;
    }
  }
  return count;
}

/// Returns the maximum loop bound of the given block, assuming it is
/// the header of a loop
static std::pair<uint64_t,uint64_t> getLoopBoundMax(const MachineBasicBlock *mbb){
  auto bounds = getLoopBounds(mbb);
  assert(bounds && "No bounds were given");
  return *bounds;
}

/// Returns the minimum/maximum possible main memory accesses the function can do
std::pair<unsigned,unsigned> MemoryAccessNormalization::getAccessBounds(MachineFunction &MF, llvm::MachineLoopInfo &LI) {
  auto accesses_counts = memoryAccessAnalysis(MF.getBlockNumbered(0), &LI,
      countAccesses, getLoopBoundMax);

  // Find the min/max number of accesses among all final blocks in the function
  unsigned max_end_accesses;
  unsigned min_end_accesses;
  const MachineBasicBlock *end_block = nullptr;
  for (auto bb_iter = MF.begin(); bb_iter != MF.end(); ++bb_iter) {
    if (bb_iter->succ_size() == 0) {
      auto block_counts = accesses_counts.first[&*bb_iter];

      min_end_accesses = block_counts.first;
      max_end_accesses = block_counts.second;

      end_block = &*bb_iter;
      break;
    }
  }
  assert(end_block && "Didn't find a final block");
  assert(min_end_accesses <= max_end_accesses && "Invalid access counts");
  LLVM_DEBUG(
    dbgs() << "\nMemory Access Analysis Results:\n";
    dbgs() << "\tSingle Blocks:\n";
    for(auto mbb_iter=MF.begin(); mbb_iter != MF.end(); mbb_iter++){
      dbgs() << "\t\t" << mbb_iter->getName() << ": "
          << accesses_counts.first[&*mbb_iter].first << ", "<< accesses_counts.first[&*mbb_iter].second << "\n";
    }
    dbgs() << "\tLoops:\n";
    for(auto entry: accesses_counts.second){
      dbgs() << "\t\t" << entry.first->getName()
          << ": Predecessor Min,Max: " << std::get<0>(entry.second).first << ", "<< std::get<0>(entry.second).second
          << " Body Min,Max: " << std::get<1>(entry.second).first << ", " << std::get<1>(entry.second).second << "\n";
      dbgs() << "\t\t\tExits:\n";
      for(auto exit_entry: std::get<2>(entry.second)){
        dbgs() << "\t\t\t\t" << exit_entry.first->getName() << ": "
            << exit_entry.second.first << ", " << exit_entry.second.second << "\n";
      }
    }
    dbgs() << "\tMin/Max Access Count in block '" << end_block->getName() << "': "
        << min_end_accesses << ", " << max_end_accesses << "\n";
  );
  return std::make_pair(min_end_accesses, max_end_accesses);
}

/// Sets the function attribute that flags that this function should use 'opposite' compensation algorithm
static void delegateToOppositePredicate(MachineFunction &MF) {
  OppositeComp++;
  MF.getFunction().addFnAttr(OppositePredicateCompensation::ENABLE_OPC_ATTR);
  LLVM_DEBUG(
    dbgs() << "Set function Attribute '" << OppositePredicateCompensation::ENABLE_OPC_ATTR << "'\n";
  );
}

bool MemoryAccessNormalization::runOnMachineFunction(MachineFunction &MF) {
  if (PatmosSinglePathInfo::isConverting(MF)){
    LLVM_DEBUG(
      dbgs() << "\n********** Patmos Memory Access Compensation **********\n";
      dbgs() << "********** Function: " << MF.getFunction().getName() << "**********\n";
      MF.dump();
    );

    if(PatmosSinglePathInfo::getCETCompAlgo() == CompensationAlgo::hybrid ||
        PatmosSinglePathInfo::getCETCompAlgo() == CompensationAlgo::counter
    ){
      auto accessBounds = getAccessBounds(MF, getAnalysis<MachineLoopInfo>());

      auto min_accesses = PatmosSinglePathInfo::isRootLike(MF)? accessBounds.first:0;
      auto max_accesses = accessBounds.second;

      if((max_accesses - min_accesses) > 0) {
        auto counter_algo_instr_need = counter_compensate(MF, max_accesses, min_accesses, false);

        // Get the number of instructions the opposite predicate compensation algorithm
        // would add to the function
        auto isPseudoRoot = MF.getInfo<PatmosMachineFunctionInfo>()->isSinglePathPseudoRoot();
        auto &cldoms = getAnalysis<ConstantLoopDominators>().dominators;
        auto opposite_algo_instr_need = 0;
        std::for_each(MF.begin(), MF.end(), [&](auto &BB){
          if(!isPseudoRoot || !cldoms.begin()->second.count(&BB)){
            opposite_algo_instr_need += countAccesses(&BB);
          }
        });

        LLVM_DEBUG(
            dbgs() << "\nInstructions needed (at least) for the 'counter' compensation algorithm:"<< counter_algo_instr_need << "\n";
            dbgs() << "Instructions needed (at least) for the 'opposite' compensation algorithm:"<< opposite_algo_instr_need << "\n";
        );

        if(counter_algo_instr_need < opposite_algo_instr_need ||
            PatmosSinglePathInfo::getCETCompAlgo() == CompensationAlgo::counter
        ) {
          CounterComp++;
          CntCmpInstr += counter_algo_instr_need;
          auto added = counter_compensate(MF, max_accesses, min_accesses, true);
          assert(counter_algo_instr_need == added);

          LLVM_DEBUG(
              dbgs() << "\********** Function: " << MF.getFunction().getName() << "**********\n";
              MF.dump();
          );
        } else {
          delegateToOppositePredicate(MF);
        }
      } else {
        LLVM_DEBUG(dbgs() << "No compensation needed\n");
        NoComp++;
        return false;
      }
    } else {
      assert(PatmosSinglePathInfo::getCETCompAlgo() == CompensationAlgo::opposite);

      delegateToOppositePredicate(MF);
    }
    LLVM_DEBUG(dbgs() << "\n********** Patmos Memory Access Compensation Finished **********\n");
    return true;
  }
  return false;
}

/// Creates a new General Purpose Virtual register in the function
static Register new_vreg(MachineFunction &MF) {
  return MF.getRegInfo().createVirtualRegister(&Patmos::RRegsRegClass);
}

/// Adds Memory Access compensation code to the given function
/// such that is results in constant execution time for the function
/// (If the function is singlepath).
/// Returns a lower bound of how many instructions were added to the final
/// binary. An exact number cannot be given, since some added instructions
/// at this point, may not results in real instructions at the end.
/// E.g. PHI instructions.
///
/// If 'should_insert' is false, no instructions are added and the function
/// isn't changed. However, the count is still performed and returned as if
/// instructions were added.
unsigned MemoryAccessNormalization::counter_compensate(MachineFunction &MF, unsigned max_accesses, unsigned min_accesses, bool should_insert) {
  assert((max_accesses - min_accesses) != 0 && "No compensation needed if no accesses are done");

  auto &LI = getAnalysis<MachineLoopInfo>();
  auto &RI = MF.getRegInfo();
  unsigned insert_count = 0;
  DebugLoc DL;

  auto compensation = memoryAccessCompensation(MF.getBlockNumbered(0), &LI, countAccesses);
  auto &cldoms = getAnalysis<ConstantLoopDominators>().dominators;;
  assert(cldoms.size() == 1 && "Constant-Loop Dominator Analysis didn't find a unique end block.");

  LLVM_DEBUG(
    dbgs() << "\nMemory Access Compensation Results:\n";
    for(auto entry: compensation) {
      dbgs() << "\t" << entry.first.first->getName() << " -> " << entry.first.second->getName()
          << ": " << entry.second << "\n";
    }
  );

  // Tracks the virtual registers used at the end of each block as the counter
  std::map<MachineBasicBlock*, Register> block_regs;
  // The blocks that have yet to be assigned a register
  std::deque<MachineBasicBlock*> worklist;
  worklist.push_back(MF.getBlockNumbered(0));

  while(!worklist.empty()) {
    auto *current = worklist.front();
    worklist.pop_front();

    LLVM_DEBUG(dbgs() << "\nCompensating in '" << current->getName() << "'\n");

    // Put any missing successors on the queue
    std::for_each(current->succ_begin(), current->succ_end(), [&](auto succ){
      if(succ != current && !block_regs.count(succ) &&
          std::find(worklist.begin(), worklist.end(), succ) == worklist.end()
      ){
        LLVM_DEBUG(dbgs() << "Enqueueing '" << current->getName() << "'\n");
        worklist.push_back(succ);
      }
    });

    if(current->pred_size() == 0) {
      // Initialize counter at entry block
      auto count_and_init_reg = compensateEntryBlock(MF, current, max_accesses, should_insert);
      insert_count += count_and_init_reg.first;
      block_regs[current] = count_and_init_reg.second;
    } else if(current->pred_size() == 1) {
      auto *pred = *current->pred_begin();
      assert(block_regs.count(pred) && "Predecessor hasn't been assigned yet");

      if(compensation.count({pred, current})) {
        // Need to decrement counter.
        auto decremented_reg = new_vreg(MF);
        block_regs[current] = decremented_reg;

        auto to_dec = compensation[{pred, current}];

        if(to_dec > 4095) {
          report_fatal_error("Not implemented compensation > 4095");
        } else {
          insert_count++;
          if(should_insert){
            BuildMI(*current, current->getFirstNonPHI(), DL, TII->get(Patmos::SUBi),
              decremented_reg)
              .addReg(Patmos::NoRegister).addImm(0)
              .addReg(block_regs[pred]) // decrement source register
              .addImm(to_dec);

            LLVM_DEBUG(dbgs() << "Inserted counter decrement in '" << current->getName() << "' by: " << to_dec << "\n");
          }
        }
      } else {
        // Not updating counter, so just use predecessors register
        block_regs[current] = block_regs[pred];
        LLVM_DEBUG(dbgs() << "Not updating counter in '" << current->getName() << "'\n");
      }
    } else if(ensureBlockAndPredsAssigned(block_regs, current, worklist)){
      insert_count += compensateMerge(current, block_regs, compensation, should_insert);
    }
  }

  insert_count += compensateEnd(MF, block_regs, max_accesses - min_accesses, should_insert);

  return insert_count;
}

/// Inserts counter initialization instruction, and if function only has
/// 1 block, also decrements the counter.
/// Returns how many instructions were added and what register the counter uses.
std::pair<unsigned, Register> MemoryAccessNormalization::compensateEntryBlock(
    MachineFunction &MF, MachineBasicBlock *entry, unsigned max_accesses, bool should_insert
) {
  DebugLoc DL;

  // If function only has one block, just use r24 directly for the counter
  auto init_reg = MF.size() == 1? Patmos::R24 : new_vreg(MF);

  auto insert_count = 1;
  if(should_insert){
	if(MF.size() == 1) {
		assert(!PatmosSinglePathInfo::isRootLike(MF) &&
			"Root-like single-block function shouldn't need decrementing");
	}
	// Single-block functions will not have any edges to decrement the counter
	// so just reduce the initial count instead.
	auto max_comp = MF.size() == 1? 0 : max_accesses;

    auto count_opcode = max_comp > 4095? Patmos::LIl : Patmos::LIi;
    BuildMI(*entry, entry->getFirstTerminator(), DL, TII->get(count_opcode),
      init_reg)
      .addReg(Patmos::NoRegister).addImm(0) // May be predicated
      .addImm(max_comp);

    LLVM_DEBUG(
      dbgs() << "Inserted counter initializer in '" << entry->getName() << "': "
        << printReg(init_reg, TRI) << " = " << max_comp << "\n";
    );
  }
  return std::make_pair(insert_count, init_reg);
}

/// Ensures that the current block and all predecessors have been assigned
/// a register.
/// The current is assigned one if not already.
/// If a predecessor is not assigned, the successors of the current block
/// are put on the worklist followed by the current block.
/// A predecessor is only ever not assigned before a block if
/// they are part of a loop.
///
/// Returns whether all the predecessors are assigned.
/// (the current block will always be assigned after calling this)
bool MemoryAccessNormalization::ensureBlockAndPredsAssigned(
  std::map<MachineBasicBlock*, Register> &block_regs,
  llvm::MachineBasicBlock *current, std::deque<MachineBasicBlock*> &worklist
) {
  if(!block_regs.count(current)) {
    // Reserve register for block
    block_regs[current] = new_vreg(*current->getParent());
  }

  auto all_preds_assigned = std::all_of(current->pred_begin(), current->pred_end(), [&](auto pred){
    return block_regs.count(pred);
  });

  if (!all_preds_assigned) {
    // Return block to queue until predecessors have been assigned
    worklist.push_back(current);
    LLVM_DEBUG(dbgs() << "Skipped '" << current->getName() << "'\n");
  }
  return all_preds_assigned;
}

/// Inserts compensation for a block that has multiple predecessors.
/// Code may be added to block itself and/or predecessors.
/// Assumes all relevant blocks have been assigned a register.
///
/// Returns number of instructions added.
unsigned MemoryAccessNormalization::compensateMerge(
  MachineBasicBlock *current,
  std::map<MachineBasicBlock*, Register> &block_regs,
  std::map<
    std::pair<const MachineBasicBlock*, const MachineBasicBlock*>,
    unsigned
  > compensation,
  bool should_insert
) {
  DebugLoc DL;
  auto *MF = current->getParent();
  auto &RI = MF->getRegInfo();
  auto insert_count = 0;

  assert(block_regs.count(current) && "No register assigned block");

  // Collect count decrements (source block, count, count register for source block)
  std::vector<std::tuple<MachineBasicBlock*, unsigned, Register>> counts;
  std::for_each(current->pred_begin(), current->pred_end(), [&](auto pred){
    assert(block_regs.count(pred) && "No register assigned block");
    counts.push_back(std::make_tuple(pred, compensation[{pred, current}], block_regs[pred]));
  });

  auto all_zero = std::all_of(counts.begin(), counts.end(), [&](auto c){
    return std::get<1>(c) == 0;
  });

  assert(!counts.empty());
  if (all_zero) {
    if (should_insert) {
      // No need to do any decrementing, so just use the existing register as PHI result
      auto phi_reg_inst_builder = BuildMI(*current, current->begin(), DL,
          TII->get(Patmos::PHI), block_regs[current]);
      for (auto count : counts) {
        phi_reg_inst_builder.addReg(std::get<2>(count));
        phi_reg_inst_builder.addMBB(std::get<0>(count));
      }
      LLVM_DEBUG(dbgs() << "Joined predecessor counts in '" << current->getName() << "': " << printReg(block_regs[current]) << "\n";);
    }
  } else {
	// Create a PHI instruction to unify the decremented counter from each predecessor
    MachineInstrBuilder phi_decremented_counter_phi;
	if(should_insert) {
		phi_decremented_counter_phi= BuildMI(*current, current->begin(), DL,
            TII->get(Patmos::PHI), block_regs[current]);
	}

    for (auto count : counts) {
      auto pred = std::get<0>(count);
      auto dec_count = std::get<1>(count);
      auto count_reg = std::get<2>(count);

      if(dec_count == 0 ) {
    	if(should_insert) {
	      phi_decremented_counter_phi.addReg(count_reg);
	      phi_decremented_counter_phi.addMBB(pred);
    	}
      } else {
		insert_count++;
		if (should_insert) {
		  // Decrement counter in source block
		  auto dec_reg = new_vreg(*MF);
		  auto dec_opcode = dec_count > 4095 ? Patmos::SUBl : Patmos::SUBi;
		  auto dec_instr = BuildMI(*pred, pred->getFirstTerminator(), DL, TII->get(dec_opcode),
		    dec_reg)
		    .addReg(Patmos::NoRegister).addImm(0)
		    .addReg(count_reg)	// Old count
		    .addImm(dec_count);	// decrement by

		  phi_decremented_counter_phi.addReg(dec_reg);
		  phi_decremented_counter_phi.addMBB(pred);

		  LLVM_DEBUG(
		    dbgs() << "Inserted decrement count in merge predecessor '" << pred->getName() << "' as: ";
		    dec_instr.getInstr()->dump();
		  );
		}
      }
    }
  }
  return insert_count;
}

/// Adds the final call to the compensation function in the end block.
/// If there are multiple blocks returning from the function, throws an error.
///
/// Returns the number of instructions added
unsigned MemoryAccessNormalization::compensateEnd(
  MachineFunction &MF, std::map<MachineBasicBlock*, Register> &block_regs,
  unsigned max_compensation, bool should_insert
) {
  bool found_end = false;
  DebugLoc DL;

  assert(std::count_if(MF.begin(), MF.end(), [](auto &mbb){return mbb.succ_size() == 0;}) <= 1
		  && "Function has multiple return blocks");

  auto end = std::find_if(MF.begin(), MF.end(), [](auto &mbb){return mbb.succ_size() == 0;});
  assert(end != MF.end() && "Couldn't find end block");
  auto *BB = &*end;

  if (should_insert) {

	// Put the max possible into r23 as input
	auto max_opcode = max_compensation > 4095 ? Patmos::LIl : Patmos::LIi;
	BuildMI(*BB, BB->getFirstTerminator(), DL, TII->get(max_opcode),
	  Patmos::R23)
	  .addReg(Patmos::NoRegister).addImm(0)
	  .addImm(max_compensation)
	  .setMIFlags(MachineInstr::FrameSetup);

	assert(block_regs.count(BB) && "End block doesn't have a counter register");

	// Replace counter virtual register with r24, so that it becomes the second input
	BuildMI(*BB, BB->getFirstTerminator(), DL, TII->get(Patmos::COPY),
	  Patmos::R24)
	  .addReg(block_regs[BB]);
	if (!PatmosSinglePathInfo::isRootLike(MF)) {
	  // Put the max possible into r24 in case the whole function is disabled
	  // forcing the maximum compensation.
	  // We negate the default predicate, such that the maximum compensation is only
	  // used when the whole function is disabled.
	  BuildMI(*BB, BB->getFirstTerminator(), DL, TII->get(max_opcode),
		Patmos::R24)
		.addReg(Patmos::NoRegister).addImm(1)
		.addImm(max_compensation);
	}

	// Insert call. Since the compensation function doesn't follow usual calling convention,
	// manually set use definitions
	// We use r23 and r24 for input to ensure the prologue/epilogue will ensure they aren't
	// changed, even if the function is disabled. Had we used r3/r4, calling the function disabled
	// would still need to change them without the prologue/epilogue automatically spilling/reloading them
	auto *call_instr = MF.CreateMachineInstr(TII->get(Patmos::CALLND), DL,true);
	auto call_instr_builder = MachineInstrBuilder(MF, call_instr)
	  .addReg(Patmos::NoRegister).addImm(0)
	  .addExternalSymbol(PatmosSinglePathInfo::getCompensationFunction())
	  .addReg(Patmos::R23, RegState::ImplicitKill)
	  .addReg(Patmos::R24, RegState::ImplicitKill)
	  .addReg(Patmos::P1,  RegState::ImplicitDefine)
	  .addReg(Patmos::P2,  RegState::ImplicitDefine)
	  .addReg(Patmos::SRB, RegState::ImplicitDefine)
	  .addReg(Patmos::SRO, RegState::ImplicitDefine)
	  .setMIFlags(MachineInstr::FrameSetup); // Ensure never predicated
	BB->insert(BB->getFirstTerminator(), call_instr);

	LLVM_DEBUG(
	  dbgs() << "\nInserted call to __patmos_main_mem_access_compensation in '" << BB->getName() << "'\n");
  }

  return 3;
}
