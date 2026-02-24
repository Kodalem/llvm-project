//==-- PatmosSinglePathInfo.cpp - Analysis Pass for SP CodeGen -----------===//
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


#include "Patmos.h"
#include "PatmosInstrInfo.h"
#include "PatmosMachineFunctionInfo.h"
#include "PatmosTargetMachine.h"
#include "llvm/IR/Function.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include "PatmosSinglePathInfo.h"

#include <map>

using namespace llvm;

#define DEBUG_TYPE "patmos-singlepath"
/// SPRootList - Option to enable single-path code generation and specify entry
///              functions. This option needs to be present even when all
///              roots are specified via attributes.
static cl::list<std::string> SPRootList(
    "mpatmos-singlepath",
    cl::value_desc("list"),
    cl::desc("Entry functions for which single-path code is generated"),
    cl::CommaSeparated);

/// EnableCET - Command line option to enable the generation of
/// constant-execution-time code (disabled by default).
static cl::opt<CompensationAlgo> EnableCET(
    "mpatmos-enable-cet",
    cl::desc("Enable the generation of constant execution-time code. Requires '-mpatmos-singlepath'."),
    cl::ValueOptional,
    // The flag was not given
    cl::init(CompensationAlgo::disabled),
    cl::values(
        clEnumValN(CompensationAlgo::hybrid,
            "hybrid","(default) Heuristically chooses the best algorithm for each situation"),
        clEnumValN(CompensationAlgo::opposite,
            "opposite", "For each main memory instruction, adds compensation instruction with opposite predicate"),
        clEnumValN(CompensationAlgo::counter,
            "counter", "A counter is maintained in the function and used in the end to compensate all at once"),
        // When nothing specific is given after the flag, this is the default
        clEnumValN(CompensationAlgo::hybrid, "", "")
    ));

static cl::opt<bool> DisablePseudoRoots(
    "mpatmos-disable-pseudo-roots",
    cl::init(false),
    cl::desc("All non-root functions' code generations assumes they might be called from a disabled path."),
    cl::Hidden);

static cl::opt<std::string> CetCompFun(
    "mpatmos-cet-compensation-function",
    cl::init(""),
    cl::desc("Which compensation function to use with 'counter' constant execution time compensation"),
    cl::Hidden);

static cl::opt<bool> EnableOldSinglePathTransform(
    "mpatmos-enable-old-singlepath",
    cl::init(false),
    cl::desc("Enables the old version of the single-path transformation."),
    cl::Hidden);

static cl::opt<bool> DisableCountlessLoops(
    "mpatmos-disable-countless-loops",
    cl::init(false),
    cl::desc("Forces single-path code to use counters on all loops to ensure constant iteration counts."),
    cl::Hidden);

///////////////////////////////////////////////////////////////////////////////

char PatmosSinglePathInfo::ID = 0;

/// createPatmosSinglePathInfoPass - Returns a new PatmosSinglePathInfo pass
/// \see PatmosSinglePathInfo
FunctionPass *
llvm::createPatmosSinglePathInfoPass(const PatmosTargetMachine &tm) {
  return new PatmosSinglePathInfo(tm);
}

///////////////////////////////////////////////////////////////////////////////


bool PatmosSinglePathInfo::isEnabled() {
  return !SPRootList.empty();
}

bool PatmosSinglePathInfo::usePseudoRoots() {
  return !DisablePseudoRoots;
}

bool PatmosSinglePathInfo::isConstant() {
  return EnableCET != CompensationAlgo::disabled;
}

CompensationAlgo PatmosSinglePathInfo::getCETCompAlgo() {
  return EnableCET;
}

const char* PatmosSinglePathInfo::getCompensationFunction() {
  if(CetCompFun.length() == 0) {
	  if(PatmosSubtarget::enableBundling()) {
		  return "__patmos_main_mem_access_compensation_di";
	  }else {
		  return "__patmos_main_mem_access_compensation";
	  }
  } else {
	  return CetCompFun.c_str();
  }

}

bool PatmosSinglePathInfo::isConverting(const MachineFunction &MF) {
  const PatmosMachineFunctionInfo *PMFI =
    MF.getInfo<PatmosMachineFunctionInfo>();
  return PMFI->isSinglePath();
}

bool PatmosSinglePathInfo::isEnabled(const Function &F) {
  return isRoot(F) || isMaybe(F) || isPseudoRoot(F);
}
bool PatmosSinglePathInfo::isEnabled(const MachineFunction &MF) {
  return PatmosSinglePathInfo::isEnabled(MF.getFunction());
}

bool PatmosSinglePathInfo::isRoot(const Function &F) {
  return F.hasFnAttribute("sp-root");
}
bool PatmosSinglePathInfo::isRoot(const MachineFunction &MF) {
  return PatmosSinglePathInfo::isRoot(MF.getFunction());
}

bool PatmosSinglePathInfo::isMaybe(const Function &F) {
  return F.hasFnAttribute("sp-maybe");
}
bool PatmosSinglePathInfo::isMaybe(const MachineFunction &MF) {
  return PatmosSinglePathInfo::isMaybe(MF.getFunction());
}

bool PatmosSinglePathInfo::isPseudoRoot(const Function &F) {
  return F.hasFnAttribute("sp-pseudo");
}
bool PatmosSinglePathInfo::isPseudoRoot(const MachineFunction &MF) {
  return PatmosSinglePathInfo::isPseudoRoot(MF.getFunction());
}

bool PatmosSinglePathInfo::isRootLike(const Function &F) {
  return isRoot(F) || isPseudoRoot(F);
}
bool PatmosSinglePathInfo::isRootLike(const MachineFunction &MF) {
  return isRootLike(MF.getFunction());
}

void PatmosSinglePathInfo::getRootNames(std::set<StringRef> &S) {
  S.insert( SPRootList.begin(), SPRootList.end() );
  S.erase("");
}

bool PatmosSinglePathInfo::useNewSinglePathTransform(){
	return !EnableOldSinglePathTransform;
}

bool PatmosSinglePathInfo::useCountlessLoops(){
	return !DisableCountlessLoops;
}

///////////////////////////////////////////////////////////////////////////////

PatmosSinglePathInfo::PatmosSinglePathInfo(const PatmosTargetMachine &tm)
  : MachineFunctionPass(ID), TM(tm),
    STC(*tm.getSubtargetImpl()),
    TII(static_cast<const PatmosInstrInfo*>(tm.getInstrInfo())), Root(0) {}


bool PatmosSinglePathInfo::doInitialization(Module &M) {
  return false;
}


bool PatmosSinglePathInfo::doFinalization(Module &M) {
  if (Root) {
    delete Root;
    Root = NULL;
  }
  return false;
}

void PatmosSinglePathInfo::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<MachineLoopInfo>();
  AU.addRequired<MachineDominatorTree>();
  AU.setPreservesAll();
  MachineFunctionPass::getAnalysisUsage(AU);
}

bool PatmosSinglePathInfo::runOnMachineFunction(MachineFunction &MF) {
  if (Root) {
    delete Root;
    Root = NULL;
  }

  // only consider function actually marked for conversion
  auto curfunc = MF.getFunction().getName();
  if ( isConverting(MF) ) {
    LLVM_DEBUG( dbgs() << "[Single-Path] Analyze '" << curfunc << "'\n" );
    analyzeFunction(MF);
  }
  // didn't modify anything
  return false;
}


void PatmosSinglePathInfo::analyzeFunction(MachineFunction &MF) {
  // we cannot handle irreducibility yet
  checkIrreducibility(MF);

  // FIXME Instead of using MachineLoopInfo for creating the Scope-tree,
  // we could use a custom algorithm (e.g. Havlak's algorithm)
  // that also checks irreducibility.
  // build the SPScope tree
  Root = SPScope::createSPScopeTree(MF, getAnalysis<MachineLoopInfo>(), TII);

  LLVM_DEBUG( print(dbgs()) );

  // XXX for extensive debugging
  //MF.viewCFGOnly();
}


void PatmosSinglePathInfo::checkIrreducibility(MachineFunction &MF) const {
  // Get dominator information
  MachineDominatorTree &DT = getAnalysis<MachineDominatorTree>();

  struct BackedgeChecker {
    MachineDominatorTree &DT;
    std::set<MachineBasicBlock *> visited, finished;
    BackedgeChecker(MachineDominatorTree &dt) : DT(dt) {}
    void dfs(MachineBasicBlock *MBB) {
      visited.insert(MBB);
      for (MachineBasicBlock::const_succ_iterator si = MBB->succ_begin(),
          se = MBB->succ_end(); si != se; ++si) {
        if (!visited.count(*si)) {
          dfs(*si);
        } else if (!finished.count(*si)) {
          // visited but not finished -> this is a backedge
          // we only support natural loops, check domination
          if (!DT.dominates(*si, MBB)) {
            report_fatal_error("Single-path code generation failed due to "
                               "irreducible CFG in '" +
                               MBB->getParent()->getFunction().getName() +
                               "'!");

          }
        }
      }
      finished.insert(MBB);
    }
  };

  BackedgeChecker c(DT);
  c.dfs(&MF.front());
}

SPScope *
PatmosSinglePathInfo::getScopeFor(const PredicatedBlock *block) const {
  return Root->findScopeOf(block);
}

void PatmosSinglePathInfo::print(raw_ostream &os, const Module *M) const {
  assert(Root);
  os << "========================================\n";
  Root->dump(os, 0, true);
  os << "========================================\n";
}

#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
void PatmosSinglePathInfo::dump() const {
  print(dbgs());
}
#endif

void PatmosSinglePathInfo::walkRoot(llvm::SPScopeWalker &walker) const {
  assert( Root != NULL );
  Root->walk(walker);
}

/// Returns the preheader and unilatch of the loop inserted by LoopCountInsert
///
/// The preheader is the single predecessors of the loop.
/// The unilatch is the block all latches have an edge to instead of the header directly
std::pair<MachineBasicBlock *, MachineBasicBlock *> PatmosSinglePathInfo::getPreHeaderUnilatch(MachineLoop *loop)
{
	auto header = loop->getHeader();
	assert(header->pred_size() == 2
			&& "Loops headers should only have a preheader and unilatch as predecessors");
	if(loop->contains(*header->pred_begin())) {
		return std::make_pair(*std::next(header->pred_begin()), *header->pred_begin());
	} else {
		return std::make_pair(*header->pred_begin(), *std::next(header->pred_begin()));
	}
}

MachineBasicBlock::iterator PatmosSinglePathInfo::getUnilatchCounterDecrementer(MachineBasicBlock *unilatch){
	auto is_dec = [](MachineInstr &instr){
		return instr.getOpcode() == Patmos::SUBi;
	};
	assert(std::count_if(unilatch->begin(), unilatch->end(), is_dec) == 1
			&& "Couldn't find unilatch unique counter decrementer");
	auto counter_decrementer = std::find_if(unilatch->begin(), unilatch->end(), is_dec);
	auto decremented_count_reg = counter_decrementer->getOperand(3).getReg();
	assert(Patmos::RRegsRegClass.contains(decremented_count_reg)
			&& "Loop counter not using general-purpose register");
	return counter_decrementer;
}

bool PatmosSinglePathInfo::needsCounter(MachineLoop *loop){
	auto header = loop->getHeader();

	return !std::any_of(header->instr_begin(), header->instr_end(), [&](auto &instr){
		return instr.getOpcode() == Patmos::PSEUDO_COUNTLESS_SPLOOP;
	});
}

Register llvm::createVirtualRegisterWithHint(MachineRegisterInfo &RI, Register hint, StringRef name) {
	auto new_vreg = RI.createVirtualRegister(&Patmos::PRegsRegClass, name);
	RI.setSimpleHint(new_vreg, hint);
	return new_vreg;
}
