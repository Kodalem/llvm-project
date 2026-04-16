
#include "VirtualizePredicates.h"
#include "Patmos.h"
#include "SinglePath/PatmosSPReduce.h"
#include "TargetInfo/PatmosTargetInfo.h"
#include "llvm/ADT/GraphTraits.h"
#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/ReachingDefAnalysis.h"
#include "llvm/Support/MathExtras.h"

using namespace llvm;

#define DEBUG_TYPE "patmos-singlepath"

char VirtualizePredicates::ID = 0;

FunctionPass *llvm::createVirtualizePredicates(const PatmosTargetMachine &tm) {
  return new VirtualizePredicates(tm);
}

bool VirtualizePredicates::runOnMachineFunction(MachineFunction &MF) {
	bool changed = false;
	// only convert function if marked
	if ( MF.getInfo<PatmosMachineFunctionInfo>()->isSinglePath()) {
		LLVM_DEBUG(
			dbgs() << "[Single-Path] Virtualize Predicates " << MF.getFunction().getName() << "\n" ;
			MF.dump();
		);

		unpredicateCounterSpillReload(MF);

		auto &RI = MF.getRegInfo();
		std::map<Register, Register> phys_to_virt;
		phys_to_virt[Patmos::P1] = createVirtualRegisterWithHint(RI, Patmos::P1, "sp.virtualized.p1");
		phys_to_virt[Patmos::P2] = createVirtualRegisterWithHint(RI, Patmos::P2, "sp.virtualized.p2");
		phys_to_virt[Patmos::P3] = createVirtualRegisterWithHint(RI, Patmos::P3, "sp.virtualized.p3");
		phys_to_virt[Patmos::P4] = createVirtualRegisterWithHint(RI, Patmos::P4, "sp.virtualized.p4");
		phys_to_virt[Patmos::P5] = createVirtualRegisterWithHint(RI, Patmos::P5, "sp.virtualized.p5");
		phys_to_virt[Patmos::P6] = createVirtualRegisterWithHint(RI, Patmos::P6, "sp.virtualized.p6");
		phys_to_virt[Patmos::P7] = createVirtualRegisterWithHint(RI, Patmos::P7, "sp.virtualized.p7");

		// Go through all instruction and replace uses of physical predicate register with their virtual
		// counterpart
		std::for_each(MF.begin(), MF.end(), [&](auto &mbb){
			std::for_each(mbb.begin(), mbb.end(), [&](MachineInstr &instr){
				for(auto &op: instr.operands()) {
					if(op.isReg() && phys_to_virt.count(op.getReg())) {
						if(instr.isCall()) {
							if(op.getReg() == Patmos::P7) {
								// Don't rename the P7 register for calls (where it is used for enabling/disabling
								// the whole function).
								assert(!PatmosSinglePathInfo::isRootLike(*getCallTarget(&instr))
										&& "Call using P7 in pseudo-root");
								assert(op.isImplicit() && "Call instruction P7 argument isn't implicit");
							} else if(op.getReg() == Patmos::P1 || op.getReg() == Patmos::P2) {
								// Don't rename P1 or P2 for Decrementing Counter Compensation call
								assert(instr.getFlag(MachineInstr::FrameSetup)
										&& "Call using P1 or P2 not for compensation function");
								assert(op.isImplicit() && "Call instruction P1 pr P2 argument isn't implicit");
							} else {
								assert(false && "Unknown call using predicates");
							}
						} else {
							op.setReg(phys_to_virt[op.getReg()]);
						}
					}
				}
			});
		});

		MF.getProperties().reset(MachineFunctionProperties::Property::IsSSA);
		MF.getProperties().reset(MachineFunctionProperties::Property::NoVRegs);
		changed |= true;

		LLVM_DEBUG(
			dbgs() << "\n[Single-Path] Virtualize Predicates End " << MF.getFunction().getName() << "\n" ;
		);
	}
	return changed;
}

/// Goes through each loop looking for its counter.
/// If the counter is spilled/reloaded, unpredicate the spill/reload instructions
/// to ensure the counter is correctly spilled/reloaded every iteration.
///
/// Also ensures if the registers used for counters are used by parallel paths
/// or callers, then they are spilled/reloaded before/after the loop or in the
/// function prologue/epilogue.
/// This ensures a disabled loop doesn't mess with other path's register values
/// and that if the function is called disabled, a loops counter doesn't overwrite
/// caller's register (which they didn't spill since the paths is disabled).
void VirtualizePredicates::unpredicateCounterSpillReload(MachineFunction &MF) {
	// MachineLoopInfo is provided via the MachineLoopInfoWrapperPass in the
	// legacy pass manager. Request the wrapper and access the MachineLoopInfo
	// instance through getLI().
	auto &LI = getAnalysis<MachineLoopInfoWrapperPass>().getLI();

	// registers used for loop counter management and their loop
	std::set<std::pair<Register, MachineLoop*>> counter_mgmt_regs;

	for(auto &header_mbb: MF){
		auto header = &header_mbb;
		auto loop = LI.getLoopFor(header);
		if(!LI.isLoopHeader(header) || !PatmosSinglePathInfo::needsCounter(loop)) continue;

		assert(getLoopBounds(header));
		auto loop_bound = getLoopBounds(header)->second;
		MachineBasicBlock *preheader, *unilatch;
		std::tie(preheader, unilatch) = PatmosSinglePathInfo::getPreHeaderUnilatch(loop);

		auto is_counter_init = [&](MachineInstr &instr){
			return
				(instr.getOpcode() == Patmos::LIi || instr.getOpcode() == Patmos::LIl) &&
				(instr.getOperand(1).isReg() && instr.getOperand(1).getReg()== Patmos::P0) &&
				(instr.getOperand(2).isImm() && instr.getOperand(2).getImm()== 0) &&
				(instr.getOperand(3).isImm() && instr.getOperand(3).getImm()== loop_bound);
		};
		assert(std::count_if(preheader->begin(), preheader->end(), is_counter_init) == 1
				&& "Ambiguous counter initializer");
		// Find loop counter initializer in preheader
		auto found_counter_init = std::find_if(preheader->begin(), preheader->end(), is_counter_init);
		assert(found_counter_init != preheader->end());
		auto counter_reg = found_counter_init->getOperand(0).getReg();

		// Check if it is spilled
		auto maybe_spill = std::next(found_counter_init);
		int frame_index;
		unsigned reg = TII->isStoreToStackSlot(*maybe_spill, frame_index);

		if(reg != 0 && counter_reg == reg) {
			// Since the counter is spilled immediately, there is no point in using a general-purpose
			// register, that might need to first be spilled so it doesn't overwrite a different paths use of the same register.
			// (it might also overrite a caller's user of the register when the function is called disabled).
			// Therefore, just use R26, since it is reserved
			found_counter_init->getOperand(0).setReg(Patmos::R26);
			LLVM_DEBUG(
				dbgs() << "Loop counter initializer switched to use R26 in 'bb."
				<< preheader->getNumber() << "." << preheader->getName() << "':";
				found_counter_init->dump();
			);

			assert(maybe_spill->getOperand(0).isReg() && maybe_spill->getOperand(0).getReg() == Patmos::NoRegister);
			assert(maybe_spill->getOperand(1).isImm() && maybe_spill->getOperand(1).getImm() == 0);
			maybe_spill->getOperand(0).setReg(Patmos::P0);
			maybe_spill->getOperand(4).setReg(Patmos::R26);
			LLVM_DEBUG(
				dbgs() << "Loop counter for loop header 'bb." << header->getNumber() << "." << header->getName()
				<< "' is spilled. Unpredicating spills/reloads:\n"
				<< "in 'bb." << preheader->getNumber() << "." << preheader->getName()
				<< "':"; maybe_spill->dump();
			);

			// Find spill/reload in unilatch and update
			auto unilatch_dec = PatmosSinglePathInfo::getUnilatchCounterDecrementer(unilatch);
			auto unilatch_dec_def = unilatch_dec->getOperand(0).getReg(); // register after decrement
			auto unilatch_dec_use = unilatch_dec->getOperand(3).getReg(); // register before decrement
			assert(unilatch_dec_def == unilatch_dec_use && "Decrement changing registers (unexpected)");

			// Update reload
			auto reload = std::prev(unilatch_dec);
			int reload_fi;
			unsigned reload_reg = TII->isLoadFromStackSlot(*reload, reload_fi);
			assert(reload_fi == frame_index && "Loop counter stack slot mismatch");
			assert(unilatch_dec_use == reload_reg && "Reload not using correct register" );
			assert(reload->getOperand(1).isReg() && reload->getOperand(1).getReg() == Patmos::NoRegister);
			assert(reload->getOperand(2).isImm() && reload->getOperand(2).getImm() == 0);
			reload->getOperand(1).setReg(Patmos::P0);
			reload->getOperand(0).setReg(Patmos::R26);
			LLVM_DEBUG(
				dbgs() << "in unilatch 'bb." << unilatch->getNumber() << "." << unilatch->getName()
				<< "':"; reload->dump();
			);

			// Update spill
			auto spill = std::next(unilatch_dec);
			int spill_fi;
			unsigned spill_reg = TII->isStoreToStackSlot(*spill, spill_fi);
			assert(spill_fi == frame_index && "Loop counter stack slot mismatch");
			assert(unilatch_dec_def == spill_reg && "Spill not using correct register" );
			assert(spill->getOperand(0).isReg() && spill->getOperand(0).getReg() == Patmos::NoRegister);
			assert(spill->getOperand(1).isImm() && spill->getOperand(1).getImm() == 0);
			spill->getOperand(0).setReg(Patmos::P0);
			spill->getOperand(4).setReg(Patmos::R26);
			LLVM_DEBUG(
				dbgs() << "in unilatch 'bb." << unilatch->getNumber() << "." << unilatch->getName()
				<< "':"; spill->dump();
			);

			unilatch_dec->getOperand(0).setReg(Patmos::R26);
			unilatch_dec->getOperand(3).setReg(Patmos::R26);
			LLVM_DEBUG(
				dbgs() << "Loop counter decrementer switched to use R26 in 'bb."
				<< unilatch->getNumber() << "." << unilatch->getName() << "':";
				unilatch_dec->dump();
			);
		} else {
			// This register will be initialized and decremented unpredicated.
			// Even if the loop is not taken. This means it might interfere with a different path's
			// use of the same register. The register might also be used by a caller, with the current
			// function being called disabled. In such a case, the caller didn't save his registers before
			// the call (since the path is disabled), which means we must spill/reload this register
			// in the prologue/epilogue.
			counter_mgmt_regs.insert(std::make_pair(counter_reg, loop));
		}
	}

	// Use the wrapper pass to get the ReachingDefInfo (RDI). The analysis
	// implementation lives in ReachingDefInfo; the wrapper exposes it to the
	// legacy pass manager.
	auto &RD = getAnalysis<ReachingDefInfoWrapperPass>().getRDI();
	PostDomTreeBase<MachineBasicBlock> PDT;
	PDT.recalculate(MF);
	std::set<std::pair<Register, MachineLoop*>> solved_counters;
	for(auto entry: counter_mgmt_regs) {
		auto reg = entry.first;
		auto loop = entry.second;
		MachineBasicBlock *preheader, *unilatch;
		std::tie(preheader, unilatch) = PatmosSinglePathInfo::getPreHeaderUnilatch(loop);

		// Get the most dominant preheaders.
		// We need to do this because there might be clang-inserted preheaders preceding our preheader
		// where the register might be spilled or moved to another register, killing our register.
		// The reaching definitions analysis will therefore not know that the definition will reach our preheader.
		while(preheader->pred_size() == 1 && (*preheader->pred_begin())->succ_size() == 1) {
			preheader = *preheader->pred_begin();
		}

		SmallPtrSet<MachineInstr*, 2> defs;
		RD.getGlobalReachingDefs(&*preheader->begin(), reg, defs);
		assert(std::all_of(defs.begin(), defs.end(), [&](MachineInstr* def){
			return def->getOperand(0).getReg() == reg; }));

		SmallPtrSet<MachineInstr*, 2> uses;
		for(auto def: defs) {
			RD.getGlobalUses(def, reg, uses);
		}

		for(auto use: uses) {
			auto use_block = use->getParent();
			if(!loop->contains(use_block) && !PDT.dominates(preheader, use_block)) {
				LLVM_DEBUG(
					dbgs() << "Problematic counter register " << printReg(reg, TRI)
							<< " in bb." << use_block->getNumber() << "." << use_block->getName()
					<< " from bb." << preheader->getNumber() << "." << preheader->getName() << "\n";
				);
				auto frame_idx = MF.getFrameInfo().CreateSpillStackObject(4, Align(4));

				// Spill in preheader
				// VReg argument is a virtual register placeholder; pass Register()
				// (NoRegister) as the helper expects a Register there (not TRI).
				TII->storeRegToStackSlot(*preheader, preheader->begin(), reg, true,
						frame_idx, &Patmos::RRegsRegClass, Register());
				preheader->begin()->getOperand(0).setReg(Patmos::P0);

				// Reload in unilatch at final exit
				// We use a pseudo-instruction because we need the final reload to be predicated
				// on the same condition as the unilatch branch. But this condition is not available
				// at this point, so we use this pseudo-instruction for now.
				BuildMI(*unilatch, unilatch->getFirstInstrTerminator(), DebugLoc(),
					TII->get(Patmos::PSEUDO_POSTLOOP_RELOAD), reg).addImm(frame_idx);

				solved_counters.insert(std::make_pair(reg,loop));
				break;
			}
		}
	}

	//Filter out any solved registers
	for(auto entry: solved_counters) {
		counter_mgmt_regs.erase(entry);
	}

	// Filter out any callee-save-regs
	const MCPhysReg *saved_regs = TRI->getCalleeSavedRegs(&MF);
	for(int idx = 0; saved_regs[idx] != 0; idx++) {
		auto is_saved = [&](auto entry){
			return entry.first == saved_regs[idx];
		};
		auto found = std::find_if(counter_mgmt_regs.begin(), counter_mgmt_regs.end(), is_saved);
		while(found != counter_mgmt_regs.end()) {
			counter_mgmt_regs.erase(found);
			found = std::find_if(counter_mgmt_regs.begin(), counter_mgmt_regs.end(), is_saved);
		}
	}

	// Add registers to prologue/epilogue
	if(!PatmosSinglePathInfo::isRootLike(MF)) {
		for(auto entry: counter_mgmt_regs) {
			auto reg = entry.first;
			LLVM_DEBUG(dbgs() << "Adding loop counter " << printReg(reg, TRI) << " to prologue/epilogue\n");
			auto frame_idx = MF.getFrameInfo().CreateSpillStackObject(4, Align(4));

			assert(Patmos::RRegsRegClass.contains(reg));
			// Spill in prologue
						TII->storeRegToStackSlot(*MF.begin(), MF.begin()->begin(), reg, true,
					frame_idx, &Patmos::RRegsRegClass, Register());
			MF.begin()->begin()->setFlag(MachineInstr::FrameSetup);

			// Spill in epilogue
			auto end_block = std::find_if(MF.begin(), MF.end(), [](auto &block){ return block.succ_size() == 0;});
			assert(end_block != MF.end());

			// Pass Register() as the VReg parameter (NoRegister) instead of TRI.
			TII->loadRegFromStackSlot(*end_block, end_block->getFirstTerminator(), reg,
									  frame_idx, &Patmos::RRegsRegClass, Register());
			auto reload = std::prev(end_block->getFirstTerminator());

			// We negate the predicate to ensure the register is only reloaded if the function
			// was disabled. (When enabled, these registers should not be preserved)
			assert(reload->getOperand(2).isImm());
			reload->getOperand(2).setImm(-1);
		}
	}
}








