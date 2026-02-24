#include "InstructionCounter.h"
#include "Patmos.h"
#include "llvm/ADT/Statistic.h"

using namespace llvm;

#define DEBUG_TYPE "patmos-singlepath"

char InstructionCounter::ID = 0;

FunctionPass *llvm::createSinglePathInstructionCounter(const PatmosTargetMachine &tm) {
  return new InstructionCounter(tm);
}

STATISTIC(SPInstructions, "Number of single instructions in single-path code (in final binary)");
STATISTIC(SPInstructionSize, "Instruction bytes in single-path code (in final binary)");

bool InstructionCounter::runOnMachineFunction(MachineFunction &MF) {
	if (PatmosSinglePathInfo::isEnabled(MF)) {
		for(auto &block: MF) {
			std::for_each(block.instr_begin(), block.instr_end(), [&](auto &instr){
				if(!instr.isPseudo() && !instr.isInlineAsm() ){
					SPInstructions++;
					SPInstructionSize += instr.getDesc().getSize();
				}
			});
		}
	}
	return false;
}

