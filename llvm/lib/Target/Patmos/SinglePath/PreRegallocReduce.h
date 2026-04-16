//===---- PreRegallocReduce.h - Reduce the CFG for Single-Path code ----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass reduces functions marked for single-path conversion.
// It operates on the information regarding SPScopes and (abstract) predicates
// obtained from PatmosSinglePathInfo, in following phases:
// (2) Code for predicate definitions is inserted in MBBs for
//     every SPScope, and instructions of their basic blocks are predicated.
// (3) The CFG is actually "reduced" or linearized, by putting alternatives
//     in sequence. This is done by a walk over the SPScope tree, which also
//     inserts MBBs around loops for predicate management,
//     setting/loading loop bounds, etc.
// (4) MBBs are merged and renumbered, as finalization step.
//
//===----------------------------------------------------------------------===//
#ifndef TARGET_PATMOS_SINGLEPATH_PREREGALLOCREDUCE_H_
#define TARGET_PATMOS_SINGLEPATH_PREREGALLOCREDUCE_H_

#include "PatmosSubtarget.h"
#include "PatmosMachineFunctionInfo.h"
#include "SinglePath/EquivalenceClasses.h"
#include "SinglePath/PatmosSinglePathInfo.h"
#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include <optional>

namespace llvm {

	class PreRegallocReduce : public MachineFunctionPass {
	private:
		const PatmosTargetMachine &TM;
		const PatmosSubtarget &STC;
		const PatmosInstrInfo *TII;
		const PatmosRegisterInfo *TRI;
		EquivalenceClasses *EQ;
		MachineRegisterInfo *RI;
		MachineLoopInfo *LI;
		// Assign each predicate (equivalence class number) to a virtual predicate register
		std::map<unsigned, Register> vreg_map;

		/// applyPredicates - Predicate instructions of MBBs in the given SPScope.
		void applyPredicates(MachineFunction *MF);

		/// insertPredDefinitions - Insert predicate register definitions
		/// to MBBs of the given SPScope.
		void insertPredDefinitions(MachineFunction*MF);

		/// Insert predicate register definitions for the given class assuming
		/// it is the class of a loop header
		void insertHeaderClassPredDefinitions(EqClass &eq_class, MachineFunction*MF);

		/// Gets the header of the given block's loop.
		/// If the block is not in a loop, the entry block is its header.
		MachineBasicBlock* get_header_of(MachineBasicBlock*mbb);

		/// Gets the header of the loop containing the equivalence class
		MachineBasicBlock* get_header_of(EqClass &eq_class);

		/// If the class contains the header, returns it.
		std::optional<MachineBasicBlock*> get_header_in_class(EqClass &eq_class);

		/// Returns whether this edge exits any loop
		bool is_exit_edge(std::pair<std::optional<MachineBasicBlock*>, MachineBasicBlock*> edge);

		Register getVreg(EqClass &eq_class);

		/// Inserts the definition of a class predicate at the entry
		void insertEntryDependencyDefinition(EqClass &eq_class, MachineFunction *MF);

		/// For each equivalence class, checks to see if its predicate is actually used
		/// to predicate any instruction. If not, removes all definitions of it.
		void removeUnusedPreds(MachineFunction &MF);

	public:
		static char ID;

		PreRegallocReduce(const PatmosTargetMachine &tm) :
			MachineFunctionPass(ID), TM(tm),
			STC(*tm.getSubtargetImpl()),
			TII(static_cast<const PatmosInstrInfo*>(tm.getInstrInfo())),
			TRI(static_cast<const PatmosRegisterInfo*>(tm.getRegisterInfo()))
		{}

		StringRef getPassName() const override {
			return "Patmos Single-Path Pre-register-allocation Reducer";
		}

	        void getAnalysisUsage(AnalysisUsage &AU) const override {
		        AU.addRequired<EquivalenceClasses>();
		        AU.addPreserved<EquivalenceClasses>();
		        AU.addRequired<MachineLoopInfoWrapperPass>();
		        AU.addPreserved<MachineLoopInfoWrapperPass>();
		        AU.addRequired<MachineModuleInfoWrapperPass>();
		        MachineFunctionPass::getAnalysisUsage(AU);
		      }

		bool runOnMachineFunction(MachineFunction &MF) override;
	};
}

#endif /* TARGET_PATMOS_SINGLEPATH_PREREGALLOCREDUCE_H_ */
