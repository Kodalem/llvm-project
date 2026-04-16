
#ifndef TARGET_PATMOS_SINGLEPATH_LINEARIZER_H_
#define TARGET_PATMOS_SINGLEPATH_LINEARIZER_H_

#include "llvm/CodeGen/MachineFunctionPass.h"
#include "PatmosSubtarget.h"
#include "PatmosSinglePathInfo.h"
#include "PatmosMachineFunctionInfo.h"

namespace llvm {

	class Linearizer : public MachineFunctionPass {
	private:
		const PatmosTargetMachine &TM;
		const PatmosSubtarget &STC;
		const PatmosInstrInfo *TII;
		const PatmosRegisterInfo *TRI;


		MachineBasicBlock* linearizeScope(SPScope *S, MachineBasicBlock* last_block = nullptr);

		void mergeMBBs(MachineFunction &MF);
	public:
		static char ID;

		Linearizer(const PatmosTargetMachine &tm) :
			MachineFunctionPass(ID), TM(tm),
			STC(*tm.getSubtargetImpl()),
			TII(static_cast<const PatmosInstrInfo*>(tm.getInstrInfo())),
			TRI(static_cast<const PatmosRegisterInfo*>(tm.getRegisterInfo()))
		{}

		StringRef getPassName() const override {
			return "Patmos Single-Path Linearizer";
		}

		void getAnalysisUsage(AnalysisUsage &AU) const override {
			AU.addRequired<PatmosSinglePathInfo>();
			// Request the MachineLoopInfo via the legacy wrapper pass
			AU.addRequired<MachineLoopInfoWrapperPass>();
			MachineFunctionPass::getAnalysisUsage(AU);
		}

		bool runOnMachineFunction(MachineFunction &MF) override;
	};
}



#endif /* TARGET_PATMOS_SINGLEPATH_LINEARIZER_H_ */
