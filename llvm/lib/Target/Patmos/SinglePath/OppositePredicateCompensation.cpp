//===-- InstructionLevelCET.cpp - Remove unused function declarations ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Implements Memory access normalization throuh Opposite Predicate Compensation
// to ensure constant execution time.
//
// Looks for any function that has been flagged 'sp-cet-opposite' and inserts the
// compensation code into it.
// Ignores singlepath functions that aren't flagged, such that it is possible
// for earlier passes to decide which functions should use this technique
// for compensation instead of another.
//
//===----------------------------------------------------------------------===//

#include "OppositePredicateCompensation.h"
#include "EquivalenceClasses.h"
#include "PatmosMachineFunctionInfo.h"
#include "TargetInfo/PatmosTargetInfo.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "optional"

#include <map>
#include <set>

using namespace llvm;

#define DEBUG_TYPE "patmos-const-exec"

STATISTIC(CompLoads,     "Number of non-phi instructions added by the 'opposite' constant execution time compensation");

char OppositePredicateCompensation::ID = 0;

const std::string OppositePredicateCompensation::ENABLE_OPC_ATTR="sp-cet-opposite";

FunctionPass *llvm::createOppositePredicateCompensationPass(const PatmosTargetMachine &tm) {
  return new OppositePredicateCompensation(tm);
}

bool OppositePredicateCompensation::runOnMachineFunction(MachineFunction &MF) {
  if (PatmosSinglePathInfo::isConverting(MF) && MF.getFunction().hasFnAttribute(ENABLE_OPC_ATTR))
  {
    LLVM_DEBUG(
	  dbgs() << "[Single-Path] Opposite Predicate Compensation " << MF.getFunction().getName() << "\n" ;
	  MF.dump();
	);

    compensate(MF);

    LLVM_DEBUG(
	  dbgs() << "[Single-Path] Opposite Predicate Compensation end " << MF.getFunction().getName() << "\n" ;
	  MF.dump();
	);
    return true;
  }

  return false;
}

void OppositePredicateCompensation::compensate(MachineFunction &MF)
{
  auto &cldoms = getAnalysis<ConstantLoopDominators>().dominators;

  std::for_each(MF.begin(), MF.end(), [&](auto &BB){
    auto dominates = PatmosSinglePathInfo::isRootLike(MF)  &&
        cldoms.begin()->second.count(&BB);

    for(auto instr_iter = BB.begin(); instr_iter != BB.end(); ++instr_iter){
      if( !dominates &&
    		  instr_iter->mayLoadOrStore() &&
			  PatmosInstrInfo::getMemType(*instr_iter) == PatmosII::MEM_M
	  ) {
	    // Always load into r0 such that you never overwrite a used register
	    auto new_instr_builder = BuildMI(BB, *instr_iter, DebugLoc(), TII->get(Patmos::LWM), Patmos::R0);
        Register pred;
        unsigned neg_flag;
        std::optional<unsigned> add_eq_class;
        if(PatmosSinglePathInfo::useNewSinglePathTransform()) {
	      // Simply negate the assigned predicate
	      new_instr_builder.addReg(instr_iter->getOperand(instr_iter->findFirstPredOperandIdx()).getReg())
	        .addImm(instr_iter->getOperand(instr_iter->findFirstPredOperandIdx()+1).getImm()==0? 1: 0);

	      // Assign the same equivalence class
	      add_eq_class = EquivalenceClasses::getEqClassNr(&*instr_iter);
        } else {
	      auto *block = getAnalysis<PatmosSinglePathInfo>().getRootScope()->findBlockOf(&BB);
	      assert(block && "MBB is not associated with a PredicatedBlock");
	      auto old_pred = block->getInstructionPredicates()[&*instr_iter];

	      // Predicates have yet to be applied to the instructions
	      new_instr_builder.addReg(Patmos::NoRegister).addImm(0);

	      // Set the new instruction to use the negated predicate
	      block->setPredicateFor(new_instr_builder.getInstr(), old_pred.first, !old_pred.second);
        }

        // We create a load using address 0 (r0), which is always a valid address.
        new_instr_builder.addReg(Patmos::R0).addImm(0);

        if(add_eq_class) {
          EquivalenceClasses::addClassMetaData(&*new_instr_builder,*add_eq_class);
        }

        CompLoads++;
      }
    }
  });
}
