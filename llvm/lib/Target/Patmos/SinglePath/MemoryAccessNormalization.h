#ifndef TARGET_PATMOS_SINGLEPATH_MEMORYACCESSNORMALIZATION_H_
#define TARGET_PATMOS_SINGLEPATH_MEMORYACCESSNORMALIZATION_H_

#include "Patmos.h"
#include "ConstantLoopDominators.h"
#include "PatmosTargetMachine.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include <deque>
#include <map>

#define DEBUG_TYPE "patmos-const-exec"

namespace llvm {

class MemoryAccessNormalization : public MachineFunctionPass {

private:
  const PatmosRegisterInfo *TRI;
  const PatmosInstrInfo *TII;

  std::pair<unsigned,unsigned> getAccessBounds(MachineFunction &, llvm::MachineLoopInfo &);
  std::pair<unsigned, Register> compensateEntryBlock(
      MachineFunction &, MachineBasicBlock *, unsigned, bool);
  bool ensureBlockAndPredsAssigned(
      std::map<MachineBasicBlock*, Register> &block_regs,
      llvm::MachineBasicBlock *current,
      std::deque<MachineBasicBlock*> &worklist);
  unsigned compensateMerge(
      MachineBasicBlock *, std::map<MachineBasicBlock*, Register> &,
      std::map<std::pair<const MachineBasicBlock*, const MachineBasicBlock*>,
              unsigned >,
      bool);
  unsigned compensateEnd(
      MachineFunction &, std::map<MachineBasicBlock*, Register> &,
      unsigned, bool);
  unsigned counter_compensate(MachineFunction &, unsigned, unsigned, bool);

public:
  static char ID;
  MemoryAccessNormalization(const PatmosTargetMachine &tm):
       MachineFunctionPass(ID),
       TII(static_cast<const PatmosInstrInfo*>(tm.getInstrInfo())),
       TRI(reinterpret_cast<const PatmosRegisterInfo*>(tm.getRegisterInfo()))
  {}

  /// getPassName - Return the pass' name.
  StringRef getPassName() const override {
    return "Patmos Single-Path Memory Access Analysis (machine code)";
  }

  /// getAnalysisUsage - Specify which passes this pass depends on
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    // Request MachineLoopInfo via its wrapper pass in the legacy pass manager
    AU.addRequired<MachineLoopInfoWrapperPass>();
    AU.addRequired<ConstantLoopDominators>();
    AU.addPreserved<ConstantLoopDominators>();
    MachineFunctionPass::getAnalysisUsage(AU);
  }

  bool doInitialization(Module &M) override {
    return false;
  }

  bool doFinalization(Module &M) override {
    return false;
  }

  bool runOnMachineFunction(MachineFunction &MF) override;
 };

}

#endif /* TARGET_PATMOS_SINGLEPATH_MEMORYACCESSNORMALIZATION_H_ */
