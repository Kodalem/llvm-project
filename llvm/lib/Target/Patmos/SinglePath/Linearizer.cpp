#include "Linearizer.h"
#include "Patmos.h"
#include "SinglePath/PatmosSPReduce.h"
#include "SinglePath/EquivalenceClasses.h"
#include "TargetInfo/PatmosTargetInfo.h"
#include "llvm/ADT/GraphTraits.h"
#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/ADT/Statistic.h"

using namespace llvm;

#define DEBUG_TYPE "patmos-singlepath"

char Linearizer::ID = 0;

FunctionPass *llvm::createSinglePathLinearizer(const PatmosTargetMachine &tm) {
  return new Linearizer(tm);
}

bool Linearizer::runOnMachineFunction(MachineFunction &MF) {
	bool changed = false;
	// only convert function if marked
	if ( MF.getInfo<PatmosMachineFunctionInfo>()->isSinglePath()) {
		auto RootScope = getAnalysis<PatmosSinglePathInfo>().getRootScope();
		LLVM_DEBUG(
			dbgs() << "[Single-Path] Linearizing " << MF.getFunction().getName() << "\n";
		);

		auto &LI = getAnalysis<MachineLoopInfoWrapperPass>().getLI();
		// Update unilatch to use the counter as condition (instead of unconditionally branching
		for(auto &header_mbb: MF){
			if(!LI.isLoopHeader(&header_mbb)) continue;
			DebugLoc DL;
			auto loop = LI.getLoopFor(&header_mbb);

			if(PatmosSinglePathInfo::needsCounter(loop)) {
				auto unilatch = PatmosSinglePathInfo::getPreHeaderUnilatch(loop).second;

				// Insert counter check in unilatch
				auto counter_decrementer = PatmosSinglePathInfo::getUnilatchCounterDecrementer(unilatch);
				auto decremented_count_reg = counter_decrementer->getOperand(3).getReg();

				auto counter_check_vreg = createVirtualRegisterWithHint(MF.getRegInfo(), Patmos::P1);
				BuildMI(*unilatch, unilatch->getFirstInstrTerminator(), DL,
					TII->get(Patmos::CMPLT), counter_check_vreg)
					.addReg(Patmos::P0).addImm(0)
					.addReg(Patmos::R0).addReg(decremented_count_reg);

				// Replace branch with back edge
				unilatch->remove(&*unilatch->getFirstInstrTerminator());
				BuildMI(*unilatch, unilatch->getFirstInstrTerminator(), DL,
					TII->get(Patmos::BR))
					.addReg(counter_check_vreg).addImm(0)
					.addMBB(&header_mbb);

				// Replace any PSEUDO_POSTLOOP_RELOAD with reload predicated on check condition
				for(auto iter = unilatch->begin(); iter != unilatch->end(); ) {
					if(iter->getOpcode() == Patmos::PSEUDO_POSTLOOP_RELOAD) {
						auto reg = iter->getOperand(0).getReg();
						assert(Patmos::RRegsRegClass.contains(reg));
						auto frame_idx = iter->getOperand(1).getImm();
						// The VReg parameter (virtual register placeholder) is expected here;
						// pass Register() (NoRegister) as helper placeholder.
						TII->loadRegFromStackSlot(*unilatch,unilatch->getFirstTerminator(), reg,
								frame_idx, &Patmos::RRegsRegClass, Register());
						std::prev(unilatch->getFirstTerminator())->getOperand(1).setReg(counter_check_vreg);
						std::prev(unilatch->getFirstTerminator())->getOperand(2).setImm(1);
						unilatch->erase(iter);
						iter = unilatch->begin();
					} else {
						iter++;
					}
				}
			} else {
				// Loops that don't use a counter just need to jump out when the unilatch is disabled.
				auto unilatch = PatmosSinglePathInfo::getPreHeaderUnilatch(loop).second;
				auto back_branch = unilatch->getFirstInstrTerminator();
				assert(back_branch->getOpcode() == Patmos::BRu);
				assert(back_branch->getOperand(0).getReg() == Patmos::NoRegister);
				assert(back_branch->getOperand(1).isImm());
				assert(back_branch->getOperand(2).getMBB() == &header_mbb);

				// We find PSEUDO_UNILATCH_EXIT_PRED to figure out which predicate should be used.
				auto found_pseudo_pred = std::find_if(unilatch->instr_begin(), unilatch->instr_end(), [&](auto &instr){
					return instr.getOpcode() == Patmos::PSEUDO_UNILATCH_EXIT_PRED;
				});
				assert(found_pseudo_pred != unilatch->instr_end());
				auto exit_pred = found_pseudo_pred->getOperand(0).getReg();
				auto exit_pred_class_nr = found_pseudo_pred->getOperand(1).getImm();
				assert(exit_pred.isVirtual() && MF.getRegInfo().getRegClass(exit_pred) == &Patmos::PRegsRegClass);

				// Erase pseudo-instruction so it doesn't need to be handled elsewhere
				unilatch->erase(found_pseudo_pred);

				// Replace branch with predicated version on the unilatch's predicate
				unilatch->remove(&*unilatch->getFirstInstrTerminator());
				auto unilatch_exit_br = BuildMI(*unilatch, unilatch->getFirstInstrTerminator(), DL,
					TII->get(Patmos::BR))
					.addReg(exit_pred).addImm(0)
					.addMBB(&header_mbb);
				EquivalenceClasses::addClassMetaData(unilatch_exit_br, exit_pred_class_nr);

				// Remove the PSEUDO_COUNTLESS_SPLOOP instruction, since it is no longer needed
				// and so we don't need to handle it in other passes
				auto found_psuedo = std::find_if(header_mbb.instr_begin(), header_mbb.instr_end(), [&](auto &instr){
					return instr.getOpcode() == Patmos::PSEUDO_COUNTLESS_SPLOOP;
				});
				assert(found_psuedo != header_mbb.instr_end());
				header_mbb.erase(found_psuedo);
			}
		}

		linearizeScope(getAnalysis<PatmosSinglePathInfo>().getRootScope());
		mergeMBBs(MF);

		// Clear all flags from general purpose register operands since they might not be correct any more.
		// E.g. if a register is killed on one path, it might not be so on the other paths. Since we are now
		// in single-path code, the kill would interfere with a live register from other paths.
		// The only flag we keep is the "implicit", since we need it for calls etc.
		// We also set the "undef" for all registers to ensure we don't get "using undefined register" errors.
		std::for_each(MF.begin(), MF.end(), [&](auto &mbb){
			std::for_each(mbb.begin(), mbb.end(), [&](MachineInstr &instr){
				for(auto &op: instr.operands()) {
					if(op.isReg() && op.isUse() && Patmos::RRegsRegClass.contains(op.getReg())) {
						op.ChangeToRegister(op.getReg(), false, op.isImplicit(), false, false, true, false);
					}
				}
			});
		});

		assert(MF.verify());

		LLVM_DEBUG(
			dbgs() << "[Single-Path] Linearizing end " << MF.getFunction().getName() << "\n";
		);
		changed |= true;
	}
	return changed;
}


MachineBasicBlock* Linearizer::linearizeScope(SPScope *S, MachineBasicBlock* last_block)
{
	auto blocks = S->getBlocksTopoOrd();

	for(auto block: blocks){
	    auto MBB = block->getMBB();
	    if (S->isSubheader(block)) {
			last_block = linearizeScope(S->findScopeOf(block), last_block);


	    } else {
			if(MBB->succ_size() == 1 && *MBB->succ_begin() == S->getHeader()->getMBB()) {
				// This is the unilatch block, don't mess with branches
			} else {
				// remove all successors
				for ( MachineBasicBlock::succ_iterator SI = MBB->succ_begin();
								SI != MBB->succ_end();
								SI = MBB->removeSuccessor(SI) )
								; // no body

				// remove the branch at the end of MBB
				TII->removeBranch(*MBB);
			}

			if (last_block) {
				// add to the last MBB as successor
				last_block->addSuccessor(MBB);
				// move in the code layout
				MBB->moveAfter(last_block);
			}
			// keep track of tail
			last_block = MBB;
	    }
	}
	return last_block;
}

void Linearizer::mergeMBBs(MachineFunction &MF) {
	LLVM_DEBUG( dbgs() << "Function before block merge:\n"; MF.dump() );
	// first, obtain the sequence of MBBs in DF order (as copy!)
	// NB: have to use the version below, as some version of libcxx will not
	// compile it (similar to
	//    http://lists.cs.uiuc.edu/pipermail/cfe-commits/Week-of-Mon-20130325/076850.html)
	//std::vector<MachineBasicBlock*> order(df_begin(&MF.front()),
	//                                      df_end(  &MF.front()));
	std::vector<MachineBasicBlock*> order;
	for (auto I = df_begin(&MF.front()), E = df_end(&MF.front()); I != E; ++I) {
		order.push_back(*I);
	}

	auto I = order.begin(), E = order.end();

	MachineBasicBlock *BaseMBB = *I;
	LLVM_DEBUG( dbgs() << "Base MBB#" << BaseMBB->getNumber() << "\n" );
	// iterate through order of MBBs
	while (++I != E) {
		// get MBB of iterator
		MachineBasicBlock *MBB = *I;

		if (BaseMBB->succ_size() > 1) {
			// we have encountered a backedge

			// Create an unconditional branch to the next block to exit the loop
			DebugLoc DL;
			BuildMI(*BaseMBB, BaseMBB->end(), DL,
				TII->get(Patmos::BRu))
				.addReg(Patmos::P0).addImm(0)
				.addMBB(MBB);

			BaseMBB = MBB;
			LLVM_DEBUG( dbgs() << "Base MBB#" << BaseMBB->getNumber() << "\n" );
		} else if (MBB->pred_size() == 1) {
			LLVM_DEBUG( dbgs() << "  Merge MBB#" << MBB->getNumber() << "\n" );
			// transfer the instructions
			BaseMBB->splice(BaseMBB->end(), MBB, MBB->begin(), MBB->end());
			// Any PHIs in MBB's successors are redirected to base
			std::for_each(MBB->succ_begin(), MBB->succ_end(), [&](auto succ){
				succ->replacePhiUsesWith(MBB, BaseMBB);
			});
			// remove the edge between BaseMBB and MBB
			BaseMBB->removeSuccessor(MBB);
			// BaseMBB gets the successors of MBB instead
			BaseMBB->transferSuccessors(MBB);

			// remove MBB from MachineFunction
			MF.erase(MBB);
		} else {
			BaseMBB = MBB;
			LLVM_DEBUG( dbgs() << "Base MBB#" << BaseMBB->getNumber() << "\n" );
		}
	}
	// invalidate order
	order.clear();
}

