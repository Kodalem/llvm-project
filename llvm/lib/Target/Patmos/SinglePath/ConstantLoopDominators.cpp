//===-- .cpp - Remove unused function declarations ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//
//
//===----------------------------------------------------------------------===//

#include "Patmos.h"
#include "SinglePath/PatmosSinglePathInfo.h"
#include "ConstantLoopDominatorAnalysis.h"
#include "ConstantLoopDominators.h"
#include "TargetInfo/PatmosTargetInfo.h"
#include "llvm/CodeGen/MachineLoopInfo.h"

using namespace llvm;


char ConstantLoopDominators::ID = 0;

/// createDataCacheAccessEliminationPass - Returns a new DataCacheAccessElimination
/// \see DataCacheAccessElimination
FunctionPass *llvm::createPatmosConstantLoopDominatorsPass() {
  return new ConstantLoopDominators();
}

static bool constantBounds(const MachineBasicBlock *mbb) {
  if(auto bounds = getLoopBounds(mbb)) {
    return bounds->first == bounds->second;
  }
  return false;
}

void ConstantLoopDominators::calculate(MachineFunction &MF, MachineLoopInfo &LI) {
  if(PatmosSinglePathInfo::isEnabled(MF)) {
    dominators = constantLoopDominatorsAnalysis(MF.getBlockNumbered(0), &LI, constantBounds);
    assert(dominators.size() == 1 && "Single-path code must have only 1 end block");
  }
}

bool ConstantLoopDominators::runOnMachineFunction(MachineFunction &MF) {
  // MachineLoopInfo is provided via the MachineLoopInfoWrapperPass in the
  // legacy pass manager. Request the wrapper and access the MachineLoopInfo
  // instance through getLI().
  calculate(MF, getAnalysis<MachineLoopInfoWrapperPass>().getLI());
  return false;
}

void ConstantLoopDominators::getAnalysisUsage(AnalysisUsage &AU) const {
  // Require the legacy wrapper pass that exposes MachineLoopInfo... again...
  // ... and again... and again...
  AU.addRequired<MachineLoopInfoWrapperPass>();
  AU.setPreservesAll();
  MachineFunctionPass::getAnalysisUsage(AU);
}


