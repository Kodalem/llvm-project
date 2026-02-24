#ifndef TARGET_PATMOS_SINGLEPATH_DATACACHEACCESSELIMINATION_H_
#define TARGET_PATMOS_SINGLEPATH_DATACACHEACCESSELIMINATION_H_

#include "Patmos.h"
#include "PatmosSinglePathInfo.h"

#define DEBUG_TYPE "patmos-singlepath"

namespace llvm {

class DataCacheAccessElimination : public MachineFunctionPass {

private:

  const PatmosTargetMachine &TM;
  const PatmosSubtarget &STC;
  const PatmosInstrInfo *TII;
  const PatmosRegisterInfo *TRI;

  void eliminateDCAccesses(MachineFunction &MF);

public:
  static char ID;
  DataCacheAccessElimination(const PatmosTargetMachine &tm):
       MachineFunctionPass(ID), TM(tm),
       STC(*tm.getSubtargetImpl()),
       TII(static_cast<const PatmosInstrInfo*>(tm.getInstrInfo())),
       TRI(static_cast<const PatmosRegisterInfo*>(tm.getRegisterInfo()))
  {}

  /// getPassName - Return the pass' name.
  StringRef getPassName() const override {
    return "Patmos Single-Path Data-Cache Access elimination (machine code)";
  }

  /// getAnalysisUsage - Specify which passes this pass depends on
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    MachineFunctionPass::getAnalysisUsage(AU);
  }

  bool doInitialization(Module &M) override {
    return false;
  }

  bool doFinalization(Module &M) override {
    return false;
  }

  bool runOnMachineFunction(MachineFunction &MF) override ;

 };

}

#endif /* TARGET_PATMOS_SINGLEPATH_DATACACHEACCESSELIMINATION_H_ */
