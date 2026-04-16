
#include "PreRegallocReduce.h"
#include "Patmos.h"
#include "TargetInfo/PatmosTargetInfo.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/MachineFunction.h"

using namespace llvm;

#define DEBUG_TYPE "patmos-singlepath"

char PreRegallocReduce::ID = 0;

FunctionPass *llvm::createPreRegallocReduce(const PatmosTargetMachine &tm) {
  return new PreRegallocReduce(tm);
}

bool PreRegallocReduce::runOnMachineFunction(MachineFunction &MF) {
	bool changed = false;
	// only convert function if marked
	if ( MF.getInfo<PatmosMachineFunctionInfo>()->isSinglePath()) {
		LLVM_DEBUG(
			dbgs() << "[Single-Path] Reducing " << MF.getFunction().getName() << "\n" ;
			MF.dump();
		);

		EQ = &getAnalysis<EquivalenceClasses>();
		RI = &MF.getRegInfo();
		// MachineLoopInfo is provided via the wrapper pass in the legacy pass manager
		LI = &getAnalysis<MachineLoopInfoWrapperPass>().getLI();
		vreg_map.clear(); // make sure other functions' maps aren't used

		applyPredicates(&MF);

		insertPredDefinitions(&MF);

		removeUnusedPreds(MF);

		MF.getProperties().reset(MachineFunctionProperties::Property::IsSSA);
		MF.getProperties().reset(MachineFunctionProperties::Property::NoVRegs);
		changed |= true;

		LLVM_DEBUG(
			dbgs() << "[Single-Path] Reducing after " << MF.getFunction().getName() << "\n" ;
			MF.dump();
		);
	}
	return changed;
}

Register PreRegallocReduce::getVreg(EqClass &eq_class)
{
	if (vreg_map.count(eq_class.number)) {
		return vreg_map[eq_class.number];
	} else {
		auto new_vreg = createVirtualRegisterWithHint(*RI);
		vreg_map[eq_class.number] = new_vreg;
		return new_vreg;
	}
}

void PreRegallocReduce::applyPredicates(MachineFunction *MF) {
	EQ->exportClassDependenciesToModule(*MF);

	auto &C = MF->getFunction().getContext();

	for(auto &mbb: *MF){
		auto eq_class = EQ->getClassFor(&mbb);
		auto predVreg = getVreg(eq_class);

		LLVM_DEBUG(
			dbgs() << "Applying predicates in bb." << mbb.getNumber() << "." << mbb.getName() << ": "
					<< eq_class.number << " -> " << printReg(predVreg, TRI, 0, &mbb.getParent()->getRegInfo()) <<"\n";
		);

		// apply predicate to all instructions in block
		for( auto MI = mbb.instr_begin(); MI != mbb.getFirstInstrTerminator(); ++MI)
		{
			if (MI->isReturn()) {
				LLVM_DEBUG( dbgs() << "    skip return: " << *MI );
				continue;
			}
			if (TII->isStackControl(&*MI)) {
				LLVM_DEBUG( dbgs() << "    skip stack control: " << *MI );
				continue;
			}
			if (MI->getFlag(MachineInstr::FrameSetup)) {
				continue;
				LLVM_DEBUG(dbgs() << "    skip frame setup: " << *MI);
			}

			if (MI->isCall() && !PatmosSinglePathInfo::isPseudoRoot(*getCallTargetMF(&*MI,
			                              getAnalysis<MachineModuleInfoWrapperPass>().getMMI()))) {
				LLVM_DEBUG( dbgs() << "    call: " << *MI );
				assert(!TII->isPredicated(*MI) && "call predicated");
				DebugLoc DL = MI->getDebugLoc();
				// copy actual preg to temporary preg
				// (this is the predicate call argument which enables/disabled the whole function)
				auto copy_instr = BuildMI(mbb, MI, DL,
					TII->get(Patmos::COPY), Patmos::P7)
				.addReg(predVreg);
				continue;
			}

			if (MI->isPredicable(MachineInstr::QueryType::IgnoreBundle) ) {
				int pred_op_idx = MI->findFirstPredOperandIdx();

				if(pred_op_idx!=-1 && MI->getOperand(pred_op_idx).getReg() == Patmos::NoRegister) {
					assert(pred_op_idx != -1);

					MachineOperand &PO1 = MI->getOperand(pred_op_idx);
					MachineOperand &PO2 = MI->getOperand(pred_op_idx+1);
					assert(PO1.isReg() && PO2.isImm() &&
									 "Unexpected Patmos predicate operand");
					PO1.setReg(predVreg);
				} else if(pred_op_idx!=-1 && MI->getOperand(pred_op_idx).getReg().isVirtual() ){
					// Instruction already predicated but not from single-path.
					// This can come from the "select" llvm instruction.
					// We must ensure the predicate depends on whether the block is
					// enabled in addition to the original condition.

					auto new_vreg = createVirtualRegisterWithHint(*RI);
					BuildMI(mbb, MI, DebugLoc(),
						TII->get(Patmos::PAND), new_vreg)
							.addReg(Patmos::P0).addImm(0) // Don't predicate
							.addReg(predVreg).addImm(0)
							.addReg(MI->getOperand(pred_op_idx).getReg()).addImm(0);
					MI->getOperand(pred_op_idx).setReg(new_vreg);
				}

				EquivalenceClasses::addClassMetaData(&*MI, eq_class.number);
			}
		}

		// Predicate the branches of the unilatches that don't use counters
		auto loop = LI->getLoopFor(&mbb);
		if(LI->isLoopHeader(&mbb) && !PatmosSinglePathInfo::needsCounter(loop)) {
			auto unilatch = PatmosSinglePathInfo::getPreHeaderUnilatch(loop).second;
			auto instr = BuildMI(*unilatch, unilatch->getFirstTerminator(), DebugLoc(),
					TII->get(Patmos::PSEUDO_UNILATCH_EXIT_PRED))
				.addReg(predVreg).addImm(eq_class.number);
		}
	}
}

bool PreRegallocReduce::is_exit_edge(std::pair<std::optional<MachineBasicBlock*>, MachineBasicBlock*> edge) {
	if(!edge.first) {
		return false;
	}
	auto source_loop = LI->getLoopFor(*edge.first);
	SmallVector<std::pair<MachineBasicBlock*, MachineBasicBlock*>, 4> exit_edges;
	if(source_loop) {
		source_loop->getExitEdges(exit_edges);
	}
	return std::count(exit_edges.begin(), exit_edges.end(), std::make_pair(*edge.first, edge.second)) != 0;
}

MachineBasicBlock* PreRegallocReduce::get_header_of(MachineBasicBlock*mbb) {
	auto entry = &*mbb->getParent()->begin();
	auto loop = LI->getLoopFor(mbb);
	return loop? loop->getHeader() : entry;
}

MachineBasicBlock* PreRegallocReduce::get_header_of(EqClass &eq_class) {
	assert(std::all_of(eq_class.members.begin(), eq_class.members.end(), [&](auto mem){
		return get_header_of(*eq_class.members.begin()) == get_header_of(mem);
	}) && "Not all members in class have the same header");
	return get_header_of(*eq_class.members.begin());
}

std::optional<MachineBasicBlock*> PreRegallocReduce::get_header_in_class(EqClass &eq_class) {
	auto header_in_class = std::find_if(eq_class.members.begin(), eq_class.members.end(),
			[&](auto member){ return LI->isLoopHeader(member) || (member->pred_size() == 0);});
	if(header_in_class != eq_class.members.end()) {
		return *header_in_class;
	} else {
		return std::nullopt;
	}
}

void PreRegallocReduce::insertEntryDependencyDefinition(
	EqClass &eq_class, MachineFunction *MF
) {
	assert(std::count_if(eq_class.dependencies.begin(), eq_class.dependencies.end(), [&](auto dep){
		return !dep.first;
	}) == 1 && "No unique dependency on loop entry");
	auto header = get_header_of(eq_class);
	assert(eq_class.dependencies.count(std::make_pair(std::nullopt, header)) && "Entry dependency not on header");

	Register initial_preg;
	if(header->pred_size() == 0) {
		// It is the function entry
		initial_preg = PatmosSinglePathInfo::isRootLike(*MF)? Patmos::P0 : Patmos::P7;
	} else {
		auto header_class = EQ->getClassFor(header);
		initial_preg = getVreg(header_class);
	}

	auto def = BuildMI(*header, header->begin(), DebugLoc(),
		TII->get(Patmos::COPY), getVreg(eq_class))
			.addReg(initial_preg);
	LLVM_DEBUG(
		dbgs() << "Inserted entry definition of predicate " << eq_class.number << ":\n\t";
		def.getInstr()->dump();
	);
}

void PreRegallocReduce::insertHeaderClassPredDefinitions(
	EqClass &eq_class, MachineFunction*MF
){
	auto header_in_class = *get_header_in_class(eq_class);

	auto class_vreg = getVreg(eq_class);
	if(header_in_class->pred_size() == 0){
		// The entry class can only have 1 initializer
		assert(eq_class.dependencies.size() == 1),
		insertEntryDependencyDefinition(eq_class, MF);
	} else {
		auto loop = LI->getLoopFor(header_in_class);
		assert(loop);

		// We use the preheader's and unilatch's predicates to initialize the predicate
		// of a class containing a header.
		MachineBasicBlock *preheader, *unilatch;
		std::tie(preheader, unilatch) = PatmosSinglePathInfo::getPreHeaderUnilatch(loop);
		auto preheader_class = EQ->getClassFor(preheader);

		// Initialize in preheader
		auto preheader_vreg = getVreg(preheader_class);
		auto def = BuildMI(*preheader, preheader->getFirstTerminator(), DebugLoc(),
									TII->get(Patmos::COPY), class_vreg)
										.addReg(preheader_vreg);
		LLVM_DEBUG(
			dbgs() << "Inserted definition of header predicate " << eq_class.number
					<< " in preheader bb." << preheader->getNumber() << "." << preheader->getName() <<":\n\t";
			def.getInstr()->dump();
		);
		auto unilatch_class = EQ->getClassFor(unilatch);
		// Initialize in unilatch
		auto unilatch_vreg = getVreg(unilatch_class);
		def = BuildMI(*unilatch, unilatch->getFirstTerminator(), DebugLoc(),
									TII->get(Patmos::COPY), class_vreg)
										.addReg(unilatch_vreg);
		LLVM_DEBUG(
			dbgs() << "Inserted definition of header predicate " << eq_class.number
					<< " in unilatch bb." << unilatch->getNumber() << "." << unilatch->getName() <<":\n\t";;
			def.getInstr()->dump();
		);
	}
}

void PreRegallocReduce::insertPredDefinitions(
	MachineFunction*MF
) {
	for(auto eq_class: EQ->getAllClasses()) {
		assert(eq_class.members.size() >= 1);
		assert(eq_class.dependencies.size() >= 1);

		if(get_header_in_class(eq_class)) {
			insertHeaderClassPredDefinitions(eq_class, MF);
			continue;
		}

		auto class_header = get_header_of(eq_class);
		auto class_vreg = getVreg(eq_class);
		bool has_exit = false;
		bool has_definition_in_class_header = false;
		// If there are more than 1 dependency edge, we can't use destructive definition
		// (i.e. may only overwrite the old value in case the definition site is enabled)
		// We don't count a dependency on the entry
		bool non_destructive_def = eq_class.dependencies.size() > ((eq_class.dependencies.count(std::make_pair(std::nullopt, class_header))) + 1);

		for(auto dep_edge: eq_class.dependencies) {
			if(!dep_edge.first) {
				insertEntryDependencyDefinition(eq_class, MF);
				continue;
			}
			auto source = *dep_edge.first;
			auto sink = dep_edge.second;
			auto source_class = EQ->getClassFor(source);
			auto source_vreg = getVreg(source_class);

			if(!eq_class.members.count(sink)) {
				auto sink_class = EQ->getClassFor(sink);
				// The edge doesn't go straight into the class.
				// Therefore, copy the target's class's predicate into this class'es predicate
				auto sink_vreg = getVreg(sink_class);

				MachineInstrBuilder def;
				if(non_destructive_def) {
					def = BuildMI(*sink, sink->getFirstTerminator(), DebugLoc(),
						TII->get(Patmos::PSET), class_vreg)
							.addReg(sink_vreg).addImm(0)
							.addUse(class_vreg, RegState::Implicit);
				} else {
					def = BuildMI(*sink, sink->getFirstTerminator(), DebugLoc(),
						TII->get(Patmos::COPY), class_vreg)
							.addReg(sink_vreg);
				}
				LLVM_DEBUG(
					dbgs() << "Inserted definition of predicate " << eq_class.number << " with non-member target "
						<< "in bb." << sink->getNumber() << "." << sink->getName() <<":\n\t";
					def.getInstr()->dump();
				);
			} else {
				auto number_of_terminators = std::distance(source->getFirstTerminator(), source->end());
				assert(
					number_of_terminators >= 1 &&
					source->getFirstTerminator()->getOpcode() == Patmos::BR &&
					source->getFirstTerminator()->getNumExplicitOperands() == 3 &&
					"Control dependency edge not sourced in a conditional branch"
				);
				// First terminator is conditional branch, second is unconditional

				// Extract condition
				auto *cond_branch = &*source->getFirstTerminator();
				auto target = PatmosInstrInfo::getBranchTarget(cond_branch);
				auto cond_reg = cond_branch->getOperand(0).getReg();
				assert(cond_reg != Patmos::NoRegister && cond_reg != Patmos::P0);
				auto cond_neg = cond_branch->getOperand(1).getImm() != 0;

				// If the conditional branch doesn't target our target, negate the condition
				cond_neg = target != sink? !cond_neg: cond_neg;

				auto target_header = get_header_of(sink);

				// We Need to initialize the predicate of an exit target to false in the header of the target
				// (unless the target and header are in the same class, since the header needs to initially be
				// enabled)
				assert((target_header == sink? eq_class.members.count(target_header) : true)
					&& "If the target of a definition is a header, it must be in the class"
				);
				assert(!eq_class.members.count(target_header));
				bool is_exit = is_exit_edge(dep_edge);
				has_exit |= is_exit;

				bool is_def_in_class_header = class_header == source;
				has_definition_in_class_header |= is_def_in_class_header;

				bool needs_implicit_use = false;
				MachineInstrBuilder def;
				if(non_destructive_def) {
					def = BuildMI(*source, source->getFirstTerminator(), DebugLoc(),
						TII->get(Patmos::PMOV), class_vreg)
							.addReg(source_vreg).addImm(0); // Only update predicate if current block is enabled
					needs_implicit_use = true;
				} else {
					def = BuildMI(*source, source->getFirstTerminator(), DebugLoc(),
						TII->get(Patmos::PAND), class_vreg)
							// If this edge exits the loop, only allow the predicate to be written
							// once (when the exit is taken). This ensures subsequent, disabled runs
							// of the loop don't overwrite the exit target's register
							// (to false, even though it should be true)
							// By using the target predicate as the negated guard, after the first
							// time the exit is taken, the predicate cannot be assigned to again
							// Remember: exit predicates are initialized to false unless their class includes the header
							.addReg(is_exit? class_vreg : Patmos::P0).addImm(is_exit?1:0)
							.addReg(source_vreg).addImm(0); // current guard as source
					needs_implicit_use = is_exit;
				}
				def.addReg(cond_reg).addImm(cond_neg? 1 : 0); // condition

				// We add implicit uses of the target vreg to ensure the register allocator knows that
				// the previous value is live, since it may not be changed. Without this LLVM would
				// think previous values are dead because it doesn't know that the instruction is predicated.
				// It will then delete previous definitions or reassign them to registers that are used by other
				// predicates.
				if(needs_implicit_use && !is_def_in_class_header) {
					def.addUse(class_vreg, RegState::Implicit);
				}
				LLVM_DEBUG(
					dbgs() << "Inserted definition of predicate " << eq_class.number
						<< " in bb." << source->getNumber() << "." << source->getName()
						<< " for bb." << sink->getNumber() << "." << sink->getName() << ":\n\t";
					def.getInstr()->dump();
				);
			}
		}

		if((non_destructive_def || has_exit) && !has_definition_in_class_header) {
			// Set predicate to false at the start of the scope (header)
			auto def = BuildMI(*class_header, class_header->getFirstTerminator(), DebugLoc(),
				TII->get(Patmos::PCLR), class_vreg)
					.addReg(Patmos::P0).addImm(0); // Never predicated
			LLVM_DEBUG(
				dbgs() << "Inserted predicate " << eq_class.number << " initial clear "
					<< "in bb." << class_header->getNumber() << "." << class_header->getName() <<":\n\t";
				def.getInstr()->dump();
			);
		}
	}
}

static bool is_used(Register pred_vreg, MachineFunction &MF) {
	for(MachineBasicBlock &block: MF) {
		for(auto instr_iter = block.instr_begin(); instr_iter != block.instr_end(); instr_iter++){

			for(auto use: instr_iter->explicit_uses()) {
				if(use.isReg()) {
					auto reg = use.getReg();
					if(reg == pred_vreg) {
						assert(reg.isVirtual() && MF.getRegInfo().getRegClass(reg) == &Patmos::PRegsRegClass);
						return true;
					}
				}
			}
		}
	}
	return false;
}
static unsigned remove_defs(Register pred_vreg, MachineFunction &MF, const PatmosRegisterInfo *TRI) {
	assert(pred_vreg.isVirtual() && MF.getRegInfo().getRegClass(pred_vreg) == &Patmos::PRegsRegClass);
	unsigned removed_count = 0;

	for(MachineBasicBlock &block: MF) {
		for(auto instr_iter = block.instr_begin(); instr_iter != block.instr_end(); ){

			if(instr_iter->definesRegister(pred_vreg, TRI)) {
				block.erase(instr_iter);
				instr_iter = block.instr_begin();
				removed_count++;
			} else {
				instr_iter++;
			}
		}
	}
	return removed_count;
}

void PreRegallocReduce::removeUnusedPreds(MachineFunction &MF) {
	bool any_removals = true;
	while(any_removals) {
		any_removals = false;
		for(auto eq_class: EQ->getAllClasses()) {
			auto vreg = getVreg(eq_class);
			if(!is_used(vreg, MF)) {
					if(remove_defs(vreg, MF, TRI) > 0){
					any_removals = true;
				}
			}
		}
	}
}
