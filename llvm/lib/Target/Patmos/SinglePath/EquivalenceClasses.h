//===---- EquivalenceClasses.h - Reduce the CFG for Single-Path code ----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//
#ifndef TARGET_PATMOS_SINGLEPATH_EQUIVALENCECLASSES_H_
#define TARGET_PATMOS_SINGLEPATH_EQUIVALENCECLASSES_H_

#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineLoopInfo.h"

#include <set>
#include <map>
#include <vector>
#include <optional>
#include <utility>

namespace llvm {

	// Represents an equivalence class
	//
	// These aren't unique and therefore pointers to EqClass shouldn't be used to
	// distinguish between classes.
	struct EqClass {
		/// The unique number assigned to this class
		unsigned number;

		/// The edges the block is control dependent on
		std::set<std::pair<std::optional<MachineBasicBlock*>,MachineBasicBlock*> > dependencies;

		// The blocks within the class
		std::set<MachineBasicBlock*> members;
	};

	class EquivalenceClasses : public MachineFunctionPass {

    private:
        // Common type aliases to simplify deeply nested templates and avoid
        // compiler parsing issues with nested commas.
        using ControlDep = std::pair<std::optional<MachineBasicBlock*>, MachineBasicBlock*>;
        using DepSet = std::set<ControlDep>;
        using BlockSet = std::set<MachineBasicBlock*>;
        using ClassData = std::pair<DepSet, BlockSet>;

	private:
		std::map<unsigned, ClassData> classes;

		// Returns a map of which classes depend on which classes (including self-dependence)
		std::map<unsigned, std::set<unsigned> > getAllClassDependencies() const;

	public:
		static char ID;

		EquivalenceClasses() :
			MachineFunctionPass(ID)
		{}

		StringRef getPassName() const override {
			return "Patmos Single-Path Equivalence Classes";
		}

		void getAnalysisUsage(AnalysisUsage &AU) const override {
                      // See: https://llvm.org/docs/doxygen/classllvm_1_1MachineLoopInfoWrapperPass.html
		      AU.addRequired<MachineLoopInfoWrapperPass>();
		      AU.addPreserved<MachineLoopInfoWrapperPass>();
		      MachineFunctionPass::getAnalysisUsage(AU);
		}

		bool runOnMachineFunction(MachineFunction &MF) override;

		std::vector<EqClass> getAllClasses() const;

		EqClass getClassFor(MachineBasicBlock*mbb) const;
		// Exports the equivalence class predecessor relations as metadata connected to the given function
		void exportClassDependenciesToModule(MachineFunction &MF);

		// Imports the metadata representing the equivalence class predecessor relations connected to the given function
		static std::map<unsigned, std::set<unsigned> > importClassDependenciesFromModule(const MachineFunction &MF);

		// Adds the given class number, as a metadata operand, to the given instruction,
		// signifying what equivalence class the instruction is predicated by
		static void addClassMetaData(MachineInstr* MI, unsigned class_nr);

		// Extracts the metadata operand signifying what equivalence class the instruction is predicated by
		static  std::optional<unsigned> getEqClassNr(const MachineInstr* MI);

		/// Returns whether the two given instructions are independent.
		/// If two instruction are dependent, it means they may be enabled at the same time.
		/// E.g., an if-else statement's two alternatives will be mutually independent but dependent on the class surrounding them.
		static bool dependentInstructions(const MachineInstr* instr1,const MachineInstr* instr2, std::map<unsigned, std::set<unsigned> > &class_predecessors);
	};
}

#endif /* TARGET_PATMOS_SINGLEPATH_EQUIVALENCECLASSES_H_ */
