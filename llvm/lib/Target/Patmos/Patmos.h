//==-- Patmos.h - Top-level interface for Patmos representation --*- C++ -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the entry points for global functions defined in
// the LLVM Patmos backend.
//
//===----------------------------------------------------------------------===//

#ifndef _LLVM_TARGET_PATMOS_H_
#define _LLVM_TARGET_PATMOS_H_

#include "MCTargetDesc/PatmosMCTargetDesc.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/ADT/ArrayRef.h"


namespace llvm {
  class PatmosTargetMachine;
  class ModulePass;
  class MachineFunctionPass;
  class formatted_raw_ostream;
  class PassRegistry;

  void initializePatmosCallGraphBuilderPass(PassRegistry&);
  void initializePatmosStackCacheAnalysisInfoPass(PassRegistry&);
  void initializePatmosPostRASchedulerPass(PassRegistry&);
  void initializePatmosPMLProfileImportPasS(PassRegistry&);

  FunctionPass *createPatmosISelDag(PatmosTargetMachine &TM, llvm::CodeGenOpt::Level OptLevel);
  ModulePass   *createPatmosSPClonePass();
  ModulePass   *createPatmosSPMarkPass(PatmosTargetMachine &tm);
  FunctionPass *createPatmosSinglePathInfoPass(const PatmosTargetMachine &tm);
  FunctionPass *createPatmosSPPreparePass(const PatmosTargetMachine &tm);
  FunctionPass *createPatmosSPBundlingPass(const PatmosTargetMachine &tm);
  FunctionPass *createOppositePredicateCompensationPass(const PatmosTargetMachine &tm);
  FunctionPass *createDataCacheAccessEliminationPass(const PatmosTargetMachine &tm);
  FunctionPass *createMemoryAccessNormalizationPass(const PatmosTargetMachine &tm);
  FunctionPass *createPatmosSPReducePass(const PatmosTargetMachine &tm);
  FunctionPass *createPreRegallocReduce(const PatmosTargetMachine &tm);
  FunctionPass *createLoopCountInsert(const PatmosTargetMachine &tm);
  FunctionPass *createVirtualizePredicates(const PatmosTargetMachine &tm);
  FunctionPass *createSinglePathLinearizer(const PatmosTargetMachine &tm);
  FunctionPass *createSPSchedulerPass(const PatmosTargetMachine &tm);
  FunctionPass *createPatmosDelaySlotFillerPass(const PatmosTargetMachine &tm,
                                                bool ForceDisable);
  FunctionPass *createPatmosFunctionSplitterPass(PatmosTargetMachine &tm);
  FunctionPass *createPatmosDelaySlotKillerPass(PatmosTargetMachine &tm);
  FunctionPass *createPatmosEnsureAlignmentPass(PatmosTargetMachine &tm);
  FunctionPass *createSinglePathInstructionCounter(const PatmosTargetMachine &tm);
  FunctionPass *createPatmosIntrinsicEliminationPass();
  FunctionPass *createPatmosConstantLoopDominatorsPass();
  FunctionPass *createEquivalenceClassesPass();
  ModulePass *createPatmosCallGraphBuilder();
  ModulePass *createPatmosStackCacheAnalysis(const PatmosTargetMachine &tm);
  ModulePass *createPatmosStackCacheAnalysisInfo(const PatmosTargetMachine &tm);
  ModulePass *createPatmosModuleExportPass(PatmosTargetMachine &TM,
                                             std::string& Filename,
                                             std::string& BitcodeFilename,
                                             ArrayRef<std::string> Roots,
                         bool SerializeAll);

  extern char &PatmosPostRASchedulerID;
} // end namespace llvm;

#endif // _LLVM_TARGET_PATMOS_H_
