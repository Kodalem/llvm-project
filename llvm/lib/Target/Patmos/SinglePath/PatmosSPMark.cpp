//===-- PatmosSPMark.cpp - Remove unused function declarations ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass marks functions for single-path conversion on machine code level.
// Functions were cloned on bitcode level (PatmosSPClone), including functions
// which could be potentially called.  These functions are library
// functions (soft floats, lowered intrinsics, etc), and calls to these were
// not visible on bitcode level. Now the respective calls are present,
// and need to be rewritten to the cloned "..._sp_" variants, which were marked
// with attribute 'sp-maybe'.
// The final decision (single-path or not) is set in PatmosMachineFunctionInfo
// (setSinglePath()). Any functions marked as 'sp-maybe' but not finally
// in the PatmosMachineFunctionInfo are "removed" again.
//
// The removal is done by erasing all basic blocks and inserting a single
// basic block with a single return instruction, the least required to
// make the compiler happy.
//
// Ideally, they should vanish completely from the final executable, but it
// seems that this cannot easily be done.
//
//===----------------------------------------------------------------------===//


#include "PatmosMachineFunctionInfo.h"
#include "PatmosSinglePathInfo.h"
#include "ConstantLoopDominators.h"
#include "TargetInfo/PatmosTargetInfo.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include <deque>

using namespace llvm;

#define DEBUG_TYPE "patmos-singlepath"

STATISTIC(NumSPTotal,   "Number of functions used in single-path");
STATISTIC(NumSPPseudo,  "Number of functions marked as pseudo-root");
STATISTIC(NumSPCall,  "Number of call instructions from & to single-path functions");
STATISTIC(NumSPPseudoCall,  "Number of call instructions from & to pseudo-root functions");
STATISTIC(NumSPCleared, "Number of sp-maybe functions deleted");

namespace {

class PatmosSPMark : public ModulePass {
private:
  typedef std::deque<MachineFunction*> Worklist;

  PatmosTargetMachine &TM;
  MachineModuleInfo *MMI; // contains map Function -> MachineFunction

  /**
   * Get the bitcode function called by a given MachineInstr.
   */
//  const Function *getCallTarget(const MachineInstr *MI) const;
//
//  /**
//   * Get the machine function called by a given MachineInstr.
//   */
//  MachineFunction *getCallTargetMF(const MachineInstr *MI) const;

  /**
   * Get the machine function called by a given MachineInstr, but if
   * no function can be found, abort with a relevant message.
   */
  MachineFunction *
  getCallTargetMFOrAbort(MachineBasicBlock::iterator MI, MachineFunction::iterator MBB);

  /**
   * Rewrite the call target of the given MachineInstr to the
   * _sp variant of the callee.
   * @pre MI is a call instruction and not already the _sp variant
   */
  void rewriteCall(MachineInstr *MI, bool pseudo_root_target);

  /**
   * Go through all instructions of the given machine function and find
   * calls that do not call a single-path function.
   * These calls are rewritten with rewriteCall().
   * Any newly reachable 'sp-maybe' functions are put on the worklist W.
   */
  void scanAndRewriteCalls(MachineFunction *MF, Worklist &W);

  /**
   * Remove all cloned 'sp-maybe' machine functions that are not marked as
   * single-path in PatmosMachineFunctionInfo.
   */
  void removeUncalledSPFunctions(Module &M);

public:
  static char ID; // Pass identification, replacement for typeid

  PatmosSPMark(PatmosTargetMachine &TM) : ModulePass(ID), TM(TM){}

  /// getPassName - Return the pass' name.
  StringRef getPassName() const override {
    return "Patmos Single-Path Mark (machine code)";
  }

  bool doInitialization(Module &M) override {
    return false;
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<MachineModuleInfoWrapperPass>();
    AU.setPreservesCFG();
  }

  bool runOnModule(Module &M) override;
};

} // end anonymous namespace

char PatmosSPMark::ID = 0;

ModulePass *llvm::createPatmosSPMarkPass(PatmosTargetMachine &tm) {
  return new PatmosSPMark(tm);
}

///////////////////////////////////////////////////////////////////////////////


bool PatmosSPMark::runOnModule(Module &M) {
  LLVM_DEBUG( dbgs() <<
         "[Single-Path] Mark functions reachable from single-path roots\n");

  MMI = &getAnalysis<MachineModuleInfoWrapperPass>().getMMI();
  assert(MMI);

  Worklist W;
  // initialize the worklist with machine functions that have either
  // sp-root function attribute
  for(auto fn_iter = M.begin(); fn_iter != M.end(); fn_iter++) {
    Function &F = *fn_iter;
    if (F.hasFnAttribute("sp-root")) {
      NumSPPseudo++;
      // get the machine-level function
      MachineFunction *MF = MMI->getMachineFunction(F);
      assert( MF );
      PatmosMachineFunctionInfo *PMFI =
        MF->getInfo<PatmosMachineFunctionInfo>();
      PMFI->setSinglePath();
      if(PatmosSinglePathInfo::usePseudoRoots()) PMFI->setSinglePathPseudoRoot();
      W.push_back(MF);
      NumSPTotal++;
    }
  }

  // process worklist
  while (!W.empty()) {
    MachineFunction *MF = W.front();
    W.pop_front();
    scanAndRewriteCalls(MF, W);
  }

  removeUncalledSPFunctions(M);

  LLVM_DEBUG( dbgs() <<
         "[Single-Path] End of single-path mark\n");
  // We either have rewritten calls or removed superfluous functions.
  return true;
}

MachineFunction *
PatmosSPMark::getCallTargetMFOrAbort(MachineBasicBlock::iterator MI, MachineFunction::iterator MBB){
  MachineFunction *MF = getCallTargetMF(&*MI);
  if (!MF) {
    errs() << "[Single-path] Cannot find ";
    bool foundSymbol = false;
    for(
      MachineInstr::mop_iterator op = MI->operands_begin();
      op != MI->operands_end();
      ++op
    ){
      if(op->isSymbol()){
        errs() << "function '" << op->getSymbolName() << "'";
        foundSymbol = true;
        break;
      }
    }
    if(!foundSymbol){
        errs() << "unknown function";
    }
    errs() << " to rewrite. Was called by '"
           << MBB->getParent()->getFunction().getName()
           << "' (indirect call?)\n";
    abort();
  }
  return MF;
}

void PatmosSPMark::scanAndRewriteCalls(MachineFunction *MF, Worklist &W) {
  LLVM_DEBUG(dbgs() << "In function '" << MF->getName() << "':\n");

  MachineDominatorTree MDT(*MF);
  MachineLoopInfo LI(MDT);
  ConstantLoopDominators CLD(*MF, LI);

  auto &bounded_doms = CLD.dominators;

  for (MachineFunction::iterator MBB = MF->begin(), MBBE = MF->end();
                                 MBB != MBBE; ++MBB) {
    for( MachineBasicBlock::iterator MI = MBB->begin(),
                                     ME = MBB->getFirstTerminator();
                                     MI != ME; ++MI) {
      if (MI->isCall()) {
        auto *original_target_MF = getCallTargetMFOrAbort(MI,MBB);

        auto *original_PMFI = original_target_MF->getInfo<PatmosMachineFunctionInfo>();

        auto pseudo_target = PatmosSinglePathInfo::usePseudoRoots() &&
                              bounded_doms.begin()->second.count(&*MBB) &&
                              MF->getInfo<PatmosMachineFunctionInfo>()->isSinglePathPseudoRoot();
        if(pseudo_target) {
          LLVM_DEBUG(dbgs() << "Call to pseudo-root: "; MI->dump());
        }
        rewriteCall(&*MI, pseudo_target);

        auto *new_target_MF = getCallTargetMF(&*MI);
        auto *new_PMFI =
            new_target_MF->getInfo<PatmosMachineFunctionInfo>();
        // we possibly have already marked the _sp variant as single-path
        // in an earlier call, if not, then set this final decision.
        if (!new_PMFI->isSinglePath()) {
          new_PMFI->setSinglePath();
          // add the new single-path function to the worklist
          W.push_back(new_target_MF);
          NumSPTotal++;

          if(pseudo_target) {
            assert(!new_PMFI->isSinglePathPseudoRoot() && "Pseudo-Root already marked");
            new_PMFI->setSinglePathPseudoRoot();
            NumSPPseudo++;
          } else if(PatmosSinglePathInfo::useNewSinglePathTransform()) {
            // Non pseudo-roots use P7 as their enable/disable predicate argument
            new_target_MF->addLiveIn(Patmos::P7, &Patmos::PRegsRegClass);
            new_target_MF->begin()->addLiveIn(Patmos::P7);
          }
        }

      }
    }
  }
}

void PatmosSPMark::rewriteCall(MachineInstr *MI, bool pseudo_root_target) {
  // get current MBB and call target
  MachineBasicBlock *MBB = MI->getParent();
  const Function *Target = getCallTarget(MI);
  // get the same function with _sp_ suffix
  SmallVector<char, 64> buf;
  const StringRef SPFuncName = Twine(
      Twine(Target->getName()) + Twine(pseudo_root_target? "_pseudo_sp_" : "_sp_")
      ).toNullTerminatedStringRef(buf);

  const Function *SPTarget = Target->getParent()->getFunction(SPFuncName);
  if (!SPTarget) {
    errs() <<  "[Single-path] function '" << SPFuncName << "' missing!\n";
    abort();
  }

  // Remove the call target operand and add a new target operand
  // with an MachineInstrBuilder. In this case, it is inserted at
  // the right place, before the implicit defs of the call.
  MI->RemoveOperand(2);
  MachineInstrBuilder MIB(*MBB->getParent(), MI);
  MIB.addGlobalAddress(SPTarget);
  if(!pseudo_root_target && PatmosSinglePathInfo::useNewSinglePathTransform()) {
	// Add P7 as implicit to ensure LLVM knows it is used by the call
	MIB.addUse(Patmos::P7, RegState::Implicit);
  }
  LLVM_DEBUG( dbgs() << "  Rewrite call: " << Target->getName()
                << " -> " << SPFuncName << "\n" );
  NumSPCall++;
  if(pseudo_root_target) NumSPPseudoCall++;
}

void PatmosSPMark::removeUncalledSPFunctions(Module &M) {
  for(auto F = M.begin(); F != M.end(); ++F) {
    if (F->hasFnAttribute("sp-maybe") || F->hasFnAttribute("sp-pseudo")) {
      // get the machine-level function
      MachineFunction *MF = MMI->getMachineFunction(*F);
      assert( MF );
      PatmosMachineFunctionInfo *PMFI =
        MF->getInfo<PatmosMachineFunctionInfo>();

      if (!PMFI->isSinglePath()) {
        LLVM_DEBUG(dbgs() << "  Remove function: " << F->getName() << "\n");
        F->eraseFromParent();
        F = M.begin();
        NumSPCleared++;
      };
    }
  }
}

void printFunction(MachineFunction &MF) {
  outs() << "Bundle function '" << MF.getFunction().getName() << "'\n";
  outs() << "Block list:\n";
  for (MachineFunction::iterator MBB = MF.begin(), MBBE = MF.end();
                                   MBB != MBBE; ++MBB) {
    for( MachineBasicBlock::iterator MI = MBB->begin(),
                                         ME = MBB->getFirstTerminator();
                                         MI != ME; ++MI) {
      outs() << "\t";
      MI->print(outs(), &(MF.getTarget()), false);
    }
    outs() << "Terminators:\n";
    for( MachineBasicBlock::iterator MI = MBB->getFirstTerminator(),
                                             ME = MBB->end();
                                             MI != ME; ++MI) {
      outs() << "\t";
      MI->print(outs(), &(MF.getTarget()), false);
    }
    outs() << "\n";
  }

  outs() <<"\n";

}
