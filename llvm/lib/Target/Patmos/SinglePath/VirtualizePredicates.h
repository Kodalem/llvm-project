//===---- VirtualizePredicates.h - Reduce the CFG for Single-Path code ----------===//
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
#ifndef TARGET_PATMOS_SINGLEPATH_VIRTUALIZEPREDICATES_H_
#define TARGET_PATMOS_SINGLEPATH_VIRTUALIZEPREDICATES_H_

#include "llvm/CodeGen/MachineFunctionPass.h"
#include "PatmosSubtarget.h"
#include "PatmosSinglePathInfo.h"
#include "PatmosMachineFunctionInfo.h"
// ReachingDefInfo wrapper pass used by the legacy pass manager.
#include "llvm/CodeGen/ReachingDefAnalysis.h"

namespace llvm {

	class VirtualizePredicates : public MachineFunctionPass {
	private:
		const PatmosInstrInfo *TII;
		const PatmosRegisterInfo *TRI;

		/// The loop counter inserted by LoopCounterInsert may be
		/// spilled/reloaded by the register allocator.
		/// Since these spills/reloads use the Patmos::NoRegister
		/// predicate by default, they will be predicated in the next steps.
		///
		/// This function finds all these spills/reloads and makes them use
		/// Patmos::P0 instead, such that they are never predicated.
		///
		/// This spill/reload could happens e.g. when a loop uses all registers
		/// meaning the register allocator will spill the counter since it is
		/// only updated and used in the unilatch.
		void unpredicateCounterSpillReload(MachineFunction &MF);
	public:
		static char ID;

		VirtualizePredicates(const PatmosTargetMachine &tm) :
			MachineFunctionPass(ID), TII(tm.getInstrInfo()), TRI(tm.getSubtargetImpl()->getRegisterInfo())
		{}

		StringRef getPassName() const override {
			return "Patmos Single-Path Virtualize Predicates";
		}

		void getAnalysisUsage(AnalysisUsage &AU) const override {
					        AU.addRequired<MachineLoopInfoWrapperPass>();
					        AU.addPreserved<MachineLoopInfoWrapperPass>();
					        // ReachingDefInfo is provided via the wrapper pass in the legacy PM.
					        AU.addRequired<ReachingDefInfoWrapperPass>();
					        AU.addPreserved<ReachingDefInfoWrapperPass>();
		        MachineFunctionPass::getAnalysisUsage(AU);
		}

		bool runOnMachineFunction(MachineFunction &MF) override;

	};
}

#endif /* TARGET_PATMOS_SINGLEPATH_VIRTUALIZEPREDICATES_H_ */
