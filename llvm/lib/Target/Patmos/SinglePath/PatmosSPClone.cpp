//===-- PatmosSPClone.cpp - Remove unused function declarations ----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass clones functions on bitcode level that might be converted to
// single-path code in the Patmos backend.
// The reason for this is that a correspondence between a MachineFunction
// and "its" bitcode function needs to be maintained.
//
// As new function calls might be inserted in the lowering phase,
// all functions marked as "used" and reachable from them are cloned as well,
// and marked with the attribute "sp-maybe".
//
// The calls inserted by lowering and unnecessarily cloned functions are
// rewritten and removed, respectively, in the PatmosSPMark pass.
//
//===----------------------------------------------------------------------===//


#include "PatmosSinglePathInfo.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include <deque>

using namespace llvm;

#define DEBUG_TYPE "patmos-singlepath"

STATISTIC(NumSPRoots,     "Number of single-path roots");
STATISTIC(NumSPClone,     "Number of function clones");

namespace {

// Functions contained in the used array but not supported for single-path
// conversion. We do neither clone nor mark them.
const char *Blacklist[] = {"_start", "abort", "fputc", "fwrite", "setjmp"};


class PatmosSPClone : public ModulePass {
private:

  typedef std::deque<Function*> Worklist;

  /// Set of function names as roots
  std::set<StringRef> SPRoots;

  /// Maintain a mapping from Function func to cloned Function func_sp_
  std::set<Function*> ClonedFunctions;

  /// Keep track of finished functions during explore.
  /// Used to detect cycles in the call graph.
  std::set<Function*> ExploreFinished;

  void loadFromGlobalVariable(SmallSet<StringRef, 32> &Result,
                              const GlobalVariable *GV) const;

  void handleRoot(Function *F);

  /**
   * Clones function F to <funcname>_sp_.
   * @return The function clone.
   */
  void cloneAndMark(Function *F);

  /**
   * Iterate through all instructions of F.
   * Explore callees of F and rewrite the calls.
   */
  void explore(Function *F);

public:
  static char ID; // Pass identification, replacement for typeid

  PatmosSPClone() : ModulePass(ID) {}

  /// getPassName - Return the pass' name.
  StringRef getPassName() const override{
    return "Patmos Single-Path Clone (bitcode)";
  }

  bool doInitialization(Module &M) override;
  bool doFinalization(Module &M) override;
  bool runOnModule(Module &M) override;
};

} // end anonymous namespace

char PatmosSPClone::ID = 0;


ModulePass *llvm::createPatmosSPClonePass() {
  return new PatmosSPClone();
}

///////////////////////////////////////////////////////////////////////////////

bool PatmosSPClone::doInitialization(Module &M) {
  PatmosSinglePathInfo::getRootNames(SPRoots);
  return false;
}

bool PatmosSPClone::doFinalization(Module &M) {
  if (!SPRoots.empty()) {
    errs() << "Following single-path roots were not found:\n";
    for (auto it=SPRoots.begin();
            it!=SPRoots.end(); ++it) {
      errs() << "'" << *it << "' ";
    }
    errs() << '\n';
    SPRoots.clear();
    report_fatal_error("Single-path code generation failed due to "
        "missing single-path entry functions!");
  }
  return false;
}

bool PatmosSPClone::runOnModule(Module &M) {
  LLVM_DEBUG( dbgs() <<
         "[Single-Path] Clone functions reachable from single-path roots\n");

  SmallSet<StringRef, 32> used;
  loadFromGlobalVariable(used, M.getGlobalVariable("llvm.used"));

  SmallSet<StringRef, 16> blacklst;
  blacklst.insert(Blacklist,
      Blacklist + (sizeof Blacklist / sizeof Blacklist[0]));

  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I) {
    Function *F = &*I;

    if (F->isDeclaration()) continue;

    // handle single-path root specified by attribute
    if (F->hasFnAttribute("sp-root")) {
      handleRoot(F);
      // function might be specified also on cmdline
      (void) SPRoots.erase(F->getName());
      continue;
    }

    // handle single-path root specified on cmdline
    if (SPRoots.count(F->getName())) {
      F->addFnAttr("sp-root");
      handleRoot(F);
      (void) SPRoots.erase(F->getName());
      continue;
    }

    // Check if used; if yes, duplicate and mark as "sp-maybe".
    if (used.count(F->getName()) && !blacklst.count(F->getName())) {
      LLVM_DEBUG( dbgs() << "Used: " << F->getName() << "\n" );
      cloneAndMark(F);
      explore(F);
      continue;
    }
  }
  return NumSPClone > 0;
}

void PatmosSPClone::loadFromGlobalVariable(SmallSet<StringRef, 32> &Result,
                                          const GlobalVariable *GV) const {
  if (!GV || !GV->hasInitializer()) return;

  // Should be an array of 'i8*'.
  const ConstantArray *InitList = dyn_cast<ConstantArray>(GV->getInitializer());
  if (InitList == 0) return;

  for (unsigned i = 0, e = InitList->getNumOperands(); i != e; ++i)
    if (const Function *F =
          dyn_cast<Function>(InitList->getOperand(i)->stripPointerCasts()))
      Result.insert(F->getName());
}

void PatmosSPClone::handleRoot(Function *F) {

  LLVM_DEBUG( dbgs() << "SPRoot " << F->getName() << "\n" );
  if (!F->hasFnAttribute(llvm::Attribute::NoInline)) {
    F->addFnAttr(llvm::Attribute::NoInline);
  }
  NumSPRoots++;

  explore(F);
}

void PatmosSPClone::cloneAndMark(Function *F) {
  ValueToValueMapTy VMap;
  Function *SPF = CloneFunction(F, VMap, NULL);
  SPF->setName(F->getName() + Twine("_sp_"));

  SPF->addFnAttr("sp-maybe");
  LLVM_DEBUG( dbgs() << "  Clone function: " << F->getName()
                << " -> " << SPF->getName() << "\n");

  // if the root attribute got cloned, remove it
  if (SPF->hasFnAttribute("sp-root")) {
    SPF->removeFnAttr("sp-root");
  }
  NumSPClone++;

  if(PatmosSinglePathInfo::usePseudoRoots()) {
    ValueToValueMapTy VMap2;
    Function *SPPseudoF = CloneFunction(F, VMap2, NULL);
    SPPseudoF->setName(F->getName() + Twine("_pseudo_sp_"));
    SPPseudoF->addFnAttr("sp-pseudo");
    LLVM_DEBUG( dbgs() << "  Clone function: " << F->getName()
                  << " -> " << SPPseudoF->getName() << "\n");
    if (SPPseudoF->hasFnAttribute("sp-root")) {
      SPPseudoF->removeFnAttr("sp-root");
    }
    NumSPClone++;
  }

  ClonedFunctions.insert(F);
}

void PatmosSPClone::explore(Function *F) {
  auto * mod = F->getParent();
  for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
      CallInst *Call = dyn_cast<CallInst>(&*I);
      if (Call && !Call->isInlineAsm()) {
        Function *Callee = Call->getCalledFunction();

        if (!Callee) {
          auto callee_name = Call->getCalledOperand()->getName();
          if (auto *alias = mod->getNamedAlias(callee_name)) {
            llvm::StringRef aliasee_name = alias->getAliasee()->getName();
            Callee = mod->getFunction(aliasee_name);
            assert(Callee && "Alias doesn't point to function");
          } else {
            report_fatal_error("Single-path code generation failed due to "
                            "indirect function call in '" + F->getName() + "'!");
          }
        }

        // skip LLVM intrinsics
        if (Callee->isIntrinsic()) continue;

        Function *SPCallee;
        if (!ClonedFunctions.count(Callee)) {
          // clone function
          cloneAndMark(Callee);
          NumSPClone++;
          // recurse into callee
          explore(Callee);
        } else {
          // check for cycle in call graph
          if (!ExploreFinished.count(Callee)) {
            report_fatal_error("Single-path code generation failed due to "
              "recursive call to '" + Callee->getName()
              + "' in '" + F->getName() + "'!");
          }
        }
      }
  }
  ExploreFinished.insert(F);
}
