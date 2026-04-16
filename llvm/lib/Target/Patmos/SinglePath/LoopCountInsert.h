//===---- LoopCountInsert.h - Reduce the CFG for Single-Path code ----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//
#ifndef TARGET_PATMOS_SINGLEPATH_LOOPCOUNDINSERT_H_
#define TARGET_PATMOS_SINGLEPATH_LOOPCOUNDINSERT_H_

#include "PatmosSubtarget.h"
#include "PatmosSinglePathInfo.h"
#include "PatmosMachineFunctionInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineLoopInfo.h"

namespace llvm {

	class LoopCountInsert : public MachineFunctionPass {
	private:
		const PatmosTargetMachine &TM;
		const PatmosSubtarget &STC;
		const PatmosInstrInfo *TII;
		const PatmosRegisterInfo *TRI;

		void doFunction(MachineFunction &MF);
		void remove_fallthroughs(MachineFunction &MF);
		void classifyLoops(MachineFunction &MF);
	public:
		static char ID;

		LoopCountInsert(const PatmosTargetMachine &tm) :
			MachineFunctionPass(ID), TM(tm),
			STC(*tm.getSubtargetImpl()),
			TII(static_cast<const PatmosInstrInfo*>(tm.getInstrInfo())),
			TRI(static_cast<const PatmosRegisterInfo*>(tm.getRegisterInfo()))
		{}

		StringRef getPassName() const override {
			return "Patmos Single-Path Loop Counter Inserter";
		}

		void getAnalysisUsage(AnalysisUsage &AU) const override {
		        // Another MachineLoopInfo... becoming... MachineLoopInfoWrapperPass
			AU.addRequired<MachineLoopInfoWrapperPass>();
			MachineFunctionPass::getAnalysisUsage(AU);
		}

		bool runOnMachineFunction(MachineFunction &MF) override;
	};
}

#endif /* TARGET_PATMOS_SINGLEPATH_LOOPCOUNDINSERT_H_ */
