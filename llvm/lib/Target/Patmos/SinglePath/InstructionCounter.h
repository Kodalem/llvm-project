
#ifndef TARGET_PATMOS_SINGLEPATH_INSTRUCTIONCOUNTER_H_
#define TARGET_PATMOS_SINGLEPATH_INSTRUCTIONCOUNTER_H_

#include "llvm/CodeGen/MachineFunctionPass.h"
#include "PatmosSinglePathInfo.h"

namespace llvm {

	class InstructionCounter : public MachineFunctionPass {
	private:
		const PatmosTargetMachine &TM;
		const PatmosSubtarget &STC;
		const PatmosInstrInfo *TII;
		const PatmosRegisterInfo *TRI;

	public:
		static char ID;

		InstructionCounter(const PatmosTargetMachine &tm) :
			MachineFunctionPass(ID), TM(tm),
			STC(*tm.getSubtargetImpl()),
			TII(static_cast<const PatmosInstrInfo*>(tm.getInstrInfo())),
			TRI(static_cast<const PatmosRegisterInfo*>(tm.getRegisterInfo()))
		{}

		StringRef getPassName() const override {
			return "Patmos Single-Path Instruction Counter";
		}

		void getAnalysisUsage(AnalysisUsage &AU) const override {
			MachineFunctionPass::getAnalysisUsage(AU);
		}

		bool runOnMachineFunction(MachineFunction &MF) override;
	};
}



#endif /* TARGET_PATMOS_SINGLEPATH_INSTRUCTIONCOUNTER_H_ */
