//==-- PatmosSinglePathInfo.h - Analysis Pass for SP CodeGen -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===//
//
// This file defines a pass to compute imformation for single-path converting
// seleced functions.
//
//===---------------------------------------------------------------------===//


#ifndef _LLVM_TARGET_PATMOS_SINGLEPATHINFO_H_
#define _LLVM_TARGET_PATMOS_SINGLEPATHINFO_H_

#include <Patmos.h>
#include <PatmosTargetMachine.h>
#include <llvm/IR/Module.h>
#include <llvm/CodeGen/MachineFunctionPass.h>

#include "SPScope.h"

#include <vector>
#include <set>
#include <map>

namespace llvm {

  class MachineLoop;

  /// Constant-execution-time compensation algorithm
  /// 'disabled' is for when no constant-excution-time was requested.
  enum CompensationAlgo {
    // No constant execution time is desired
    disabled,
    // Algorithms
    hybrid, opposite, counter
  };

///////////////////////////////////////////////////////////////////////////////

  /// PatmosSinglePathInfo - Single-Path analysis pass
  class PatmosSinglePathInfo : public MachineFunctionPass {
    private:
      const PatmosTargetMachine &TM;
      const PatmosSubtarget &STC;
      const PatmosInstrInfo *TII;

      /// Set of functions yet to be analyzed
      std::set<std::string> FuncsRemain;

      /// Root SPScope
      SPScope *Root;

      /// Analyze a given MachineFunction
      void analyzeFunction(MachineFunction &MF);

      /// Fail with an error if MF is irreducible.
      void checkIrreducibility(MachineFunction &MF) const;

    public:
      /// Pass ID
      static char ID;

      /// isEnabled - Return true if there are functions specified to
      /// to be converted to single-path code.
      static bool isEnabled();

      /// isEnabled - Return true if a particular function is specified to
      /// to be converted to single-path code.
      static bool isEnabled(const Function &F);
      static bool isEnabled(const MachineFunction &MF);

      static bool isConverting(const MachineFunction &MF);

      static bool isRoot(const Function &F);
      static bool isRoot(const MachineFunction &MF);

      static bool isReachable(const Function &F);
      static bool isReachable(const MachineFunction &MF);

      static bool isMaybe(const Function &F);
      static bool isMaybe(const MachineFunction &MF);

      static bool isPseudoRoot(const Function &F);
      static bool isPseudoRoot(const MachineFunction &MF);

      static bool isRootLike(const Function &F);
      static bool isRootLike(const MachineFunction &MF);

      static const char* getCompensationFunction();

      /// isConstant - Return true if should produce constant execution-time,
      /// single-path code.
      static bool isConstant();

      /// getCETCompAlgo - Return which constant-execution-time algorithm
      /// was chosen.
      static CompensationAlgo getCETCompAlgo();

      /// getRootNames - Fill a set with the names of
      /// single-path root functions
      static void getRootNames(std::set<StringRef> &S);

      /// Whether should use pseudo-roots single-path functions.
      /// A pseudo root function is one that is guaranteed to be called
      /// from an enabled path of a root or another pseudo-root.
      static bool usePseudoRoots();

      /// Whether should use the new Single-Path transformation.
      static bool useNewSinglePathTransform();

      /// Whether new single-path transformation may avoid using counters
      /// to ensure loops iterate a fixed number of times where possible.
      static bool useCountlessLoops();

      /// Returns the preheader and unilatch of the loop inserted by LoopCountInsert.
      ///
      /// The preheader is the single predecessors of the loop.
      /// The unilatch is the block all latches have an edge to instead of the header directly
      static std::pair<MachineBasicBlock *, MachineBasicBlock *> getPreHeaderUnilatch(MachineLoop *loop);

      /// Returns whether the given loop needs a counter to ensure it iterates a constant number of times.
      /// This is decided by the presence of PSEUDO_COUNTLESS_SPLOOP in the loops header.
      static bool needsCounter(MachineLoop *loop);

      /// Returns the counter decrementer in the given unilatch (as inserted by LoopCountInsert).
      static MachineBasicBlock::iterator getUnilatchCounterDecrementer(MachineBasicBlock *unilatch);

      /// PatmosSinglePathInfo - Constructor
      PatmosSinglePathInfo(const PatmosTargetMachine &tm);

      /// doInitialization - Initialize SinglePathInfo pass
      bool doInitialization(Module &M) override;

      /// doFinalization - Finalize SinglePathInfo pass
      bool doFinalization(Module &M) override;

      /// getAnalysisUsage - Specify which passes this pass depends on
      void getAnalysisUsage(AnalysisUsage &AU) const override;

      /// runOnMachineFunction - Run the SP converter on the given function.
      bool runOnMachineFunction(MachineFunction &MF) override;

      /// getPassName - Return the pass' name.
      StringRef getPassName() const override{
        return "Patmos Single-Path Info";
      }

      /// print - Convert to human readable form
      virtual void print(raw_ostream &OS, const Module* = 0) const;

#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
      /// dump - Dump the state of this pass
      virtual void dump() const;
#endif

      /// isToConvert - Return true if the function should be if-converted
      bool isToConvert(MachineFunction &MF) const;

      /// getRootScope - Return the Root SPScope for this function
      SPScope *getRootScope() const { return Root; }

      /// getScopeFor - Return the innermost scope of an MBB
      SPScope *getScopeFor(const PredicatedBlock *MBB) const;

      /// walkRoot - Walk the top-level SPScope
      void walkRoot(SPScopeWalker &walker) const;
  };

///////////////////////////////////////////////////////////////////////////////

  // Allow clients to walk the list of nested SPScopes
  template <> struct GraphTraits<PatmosSinglePathInfo*> {
    typedef SPScope NodeType;
    typedef SPScope::child_iterator ChildIteratorType;

    static NodeType *getEntryNode(const PatmosSinglePathInfo *PSPI) {
      return PSPI->getRootScope();
    }
    static inline ChildIteratorType child_begin(NodeType *N) {
      return N->child_begin();
    }
    static inline ChildIteratorType child_end(NodeType *N) {
      return N->child_end();
    }
  };

  // Creates a new virtual predicate register and sets the given hint.
  //
  // There is a bug either in the LLVM register allocator or in the Patmos backend
  // that requires all predicate virtual register to have a hint. If not, the allocator
  // will fail an assertion looking for it. When we have no good hint, NoRegister will work.
  Register createVirtualRegisterWithHint(MachineRegisterInfo &RI, Register hint = Patmos::NoRegister, StringRef name="");


} // end of namespace llvm

#endif // _LLVM_TARGET_PATMOS_SINGLEPATHINFO_H_
