#ifndef TARGET_PATMOS_SINGLEPATH_OPPOSITEPREDICATECOMPENSATION_H_
#define TARGET_PATMOS_SINGLEPATH_OPPOSITEPREDICATECOMPENSATION_H_

#include "Patmos.h"
#include "PatmosSinglePathInfo.h"
#include "ConstantLoopDominators.h"
#include "llvm/IR/Metadata.h"

namespace llvm {

class OppositePredicateCompensation : public MachineFunctionPass {

private:

  const PatmosInstrInfo *TII;

public:
  static char ID;

  // The function attribute that flags that the function
  // needs to be compensated using opposite predicate compensation
  static const std::string ENABLE_OPC_ATTR;

  OppositePredicateCompensation(const PatmosTargetMachine &tm):
       MachineFunctionPass(ID),
       TII(static_cast<const PatmosInstrInfo*>(tm.getInstrInfo()))
  {}

  /// getPassName - Return the pass' name.
  StringRef getPassName() const override {
    return "Patmos Single-Path Opposite Predicate Compensation";
  }

  /// getAnalysisUsage - Specify which passes this pass depends on
  void getAnalysisUsage(AnalysisUsage &AU) const override {
	if(!PatmosSinglePathInfo::useNewSinglePathTransform()) {
	    AU.addRequired<PatmosSinglePathInfo>();
	}
	AU.addPreserved<PatmosSinglePathInfo>();
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

  bool runOnMachineFunction(MachineFunction &MF) override ;

  /// Inserts Memory access compensation code to ensure constant execution times.
  ///
  /// Does so by looking for any instruction that loads/stores to main memory,
  /// and creates a main-memory load with the same predicate as the original instruction,
  /// except negated.
  /// This ensures exactly one of them is always enabled at runtime, ensuring the
  /// latency for accessing main memory is always incurred.
  void compensate(MachineFunction &MF);
};

}

#endif /* TARGET_PATMOS_SINGLEPATH_OPPOSITEPREDICATECOMPENSATION_H_ */
