
#include "EquivalenceClasses.h"
#include "Patmos.h"
#include "SinglePath/PatmosSPReduce.h"
#include "SinglePath/ConstantLoopDominatorAnalysis.h" // for "get_intersection"
#include "SinglePath/FCFGPostDom.h"
#include "TargetInfo/PatmosTargetInfo.h"
#include "llvm/ADT/GraphTraits.h"
#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/IR/Dominators.h"

#include <deque>

using namespace llvm;

#define DEBUG_TYPE "patmos-singlepath"

STATISTIC( NrEquivalenceClasses, "Number of equivalence classes in single-path code");

char EquivalenceClasses::ID = 0;

FunctionPass *llvm::createEquivalenceClassesPass() {
  return new EquivalenceClasses;
}

bool EquivalenceClasses::runOnMachineFunction(MachineFunction &MF) {
	if(!PatmosSinglePathInfo::isEnabled(MF)) {
		return false;
	}

	LLVM_DEBUG(
		dbgs() << "\n\n[Single-Path] Equivalence Classes '" << MF.getFunction().getName() << "'\n" ;
		MF.dump();
	);

        auto &LI = getAnalysis<llvm::MachineLoopInfoWrapperPass>().getLI();

	FCFGPostDom doms(MF, LI);
	LLVM_DEBUG(doms.print(dbgs()));

 	std::map<
 		// X
 		MachineBasicBlock*,
 		// Set of {Y->Z} control dependencies
 		std::set<std::pair<std::optional<MachineBasicBlock*>,MachineBasicBlock*> >
 	>  deps;
	doms.get_control_dependencies(deps);

	LLVM_DEBUG(
		dbgs() << "New Control Dependencies:\n";
		for(auto entry: deps){
			dbgs() << "bb." << entry.first->getNumber() << ": ";
			for(auto edge: entry.second){
				dbgs() << "(";
				if(edge.first) {
					dbgs() << "bb." << (*edge.first)->getNumber() ;
				}
				dbgs() << " -> bb." << edge.second->getNumber()  << "), ";
			}
			dbgs() << "\n";
		}
	);

	// Find equivalence classes
	// Blocks are in the same equivalence class when they have the same control dependencies
	unsigned next_class_number = 0;
	classes.clear();

	std::for_each(MF.begin(), MF.end(), [&](auto &mbb){
		// Check if class already exists
		auto found = std::find_if(classes.begin(), classes.end(), [&](auto entry){
			return deps[&mbb] == entry.second.first;
		});

		if(found != classes.end()) {
			found->second.second.insert(&mbb);
		} else {
			classes[next_class_number++] = std::make_pair(deps[&mbb], std::set<MachineBasicBlock*>({&mbb}));
		}
	});
	NrEquivalenceClasses += classes.size();

	LLVM_DEBUG(
		dbgs() << "Equivalence Classes:\n";
		for(auto entry: classes){
			dbgs() << "(" << entry.first << "):\n\tControl Dependencies: {";
			for(auto edge: entry.second.first){
				dbgs() << "(";
				if(edge.first) {
					dbgs() << "bb." << (*edge.first)->getNumber() ;
				}
				dbgs() << " -> bb." << edge.second->getNumber()  << "), ";
			}
			dbgs() << "}\n\tBlocks:{";
			for(auto block: entry.second.second){
				dbgs() << "bb." << block->getNumber()  << ", ";
			}
			dbgs() << "}\n";
		}
		auto class_dependencies = getAllClassDependencies();
		dbgs() << "Equivalence Class Dependencies:\n";
		for(auto entry: class_dependencies) {
			dbgs() << entry.first << ": ";
			for(auto parent: entry.second) {
				dbgs() << parent << ", ";
			}
			dbgs() << "\n";
		}
	);

	return false;
}

std::vector<EqClass> EquivalenceClasses::getAllClasses() const {
	std::vector<EqClass> result;

	for(auto entry: classes){
		result.push_back( EqClass{
			entry.first, entry.second.first, entry.second.second
		});
	}

	return result;
}

EqClass EquivalenceClasses::getClassFor(MachineBasicBlock *mbb) const{
	auto found = std::find_if(classes.begin(), classes.end(), [&](auto entry){
		return entry.second.second.count(mbb);
	});

	assert(found != classes.end() && "No class for block");

	return EqClass{
		found->first, found->second.first, found->second.second
	};
}

std::map<unsigned, std::set<unsigned> > EquivalenceClasses::getAllClassDependencies() const {
	// Two classes are dependent if either of them is reachable from the other without
	// taking any back edges

	MachineDominatorTree MDT(*(*classes.begin()->second.second.begin())->getParent());
	MachineLoopInfo LI(MDT);

 	std::map<unsigned, std::set<unsigned> > dependencies;

	// For each class we use breadth first search while ignoring back edges.
	// For any pair of classes reachable become dependent on each other.
	for(auto entry: classes) {
		auto class_nr = entry.first;
		auto class_blocks = entry.second.second;

		// Start from all the class's blocks
		std::deque<MachineBasicBlock*> worklist;
		for(auto block: class_blocks) {
			worklist.push_back(block);
		}

		std::set<MachineBasicBlock*> donelist;
		while(!worklist.empty()) {
			auto block = worklist.front();
			worklist.pop_front();
			if(donelist.count(block)) continue;
			donelist.insert(block);

			auto block_class_nr = EquivalenceClasses::getClassFor(block).number;
			dependencies[class_nr].insert(block_class_nr);
			dependencies[block_class_nr].insert(class_nr);

			// returns whether the edge between the two blocks is a back edge.
			auto is_back_edge = [&](auto source, auto target){
				if(LI.isLoopHeader(target)) {
					if(auto target_loop = LI.getLoopFor(target)) {
						auto preheader = PatmosSinglePathInfo::getPreHeaderUnilatch(target_loop).first;
						return source != preheader;
					} else {
						// Root loop (NULL) cannot have back edge
					}
				} else {
					// Back edges must target a header
				}
				return false;
			};

			// Continue search through successors
			for(auto succ: block->successors()) {
				if(!is_back_edge(block, succ) && !donelist.count(succ)) {
					worklist.push_back(succ);
				}
			}
		}
	}

	return dependencies;
}

static MDNode* unsigned_md(unsigned x, LLVMContext &C) {
	return MDNode::get(C, ConstantAsMetadata::get(ConstantInt::get(C, llvm::APInt(64, x, false))));
}

static const char* CLASS_DEPENDENCY_FUNCTION_MD_NAME = "llvm.patmos.equivalence.class.dependencies";

void EquivalenceClasses::exportClassDependenciesToModule(MachineFunction &MF) {

	auto &F= MF.getFunction();
	LLVMContext &C = F.getContext();
	std::vector<Metadata*> mds;
	std::map<unsigned, MDNode*> result;

	for(auto entry: getAllClassDependencies()) {

		std::vector<Metadata*> dependency_md_list;
		for(auto dep: entry.second) {
			dependency_md_list.push_back(unsigned_md(dep, C));
		}
		MDNode* dependency_md = MDTuple::get(C, ArrayRef<Metadata*>(dependency_md_list));
		std::vector<Metadata*> entry_md_list;
		auto class_md = unsigned_md(entry.first, C);
		result[entry.first] = class_md;
		entry_md_list.push_back(class_md);
		entry_md_list.push_back(dependency_md);
		mds.push_back(MDTuple::get(C, entry_md_list));
	}
	F.setMetadata(CLASS_DEPENDENCY_FUNCTION_MD_NAME, MDTuple::get(C, mds));
}

std::map<unsigned, std::set<unsigned> > EquivalenceClasses::importClassDependenciesFromModule(const MachineFunction &MF) {
	LLVM_DEBUG(
		dbgs() << "Equivalence Class Dependencies:\n";
	);
 	std::map<unsigned, std::set<unsigned> > result;

	auto &F= MF.getFunction();
	LLVMContext &C = F.getContext();
	assert(F.getMetadata(CLASS_DEPENDENCY_FUNCTION_MD_NAME) && "Equivalence class dependencies not exported to metadata");

	auto class_tree_md = dyn_cast<MDTuple>(F.getMetadata(CLASS_DEPENDENCY_FUNCTION_MD_NAME));

	for(auto i = 0; i<class_tree_md->getNumOperands(); i++) {
		auto class_dep_md_entry = dyn_cast<MDTuple>(class_tree_md->getOperand(i));
		assert(class_dep_md_entry->getNumOperands() == 2);

		auto get_val = [&](auto &md){
			return cast<ConstantInt>(dyn_cast<ConstantAsMetadata>(dyn_cast<MDNode>(md)->getOperand(0))->getValue())->getSExtValue();
		};

		auto class_nr = get_val(class_dep_md_entry->getOperand(0));
		LLVM_DEBUG(
			dbgs() << class_nr << "<"; class_dep_md_entry->getOperand(0)->printAsOperand(dbgs(), F.getParent());
			dbgs() << ">: ";
		);
		auto dep_md = dyn_cast<MDTuple>(class_dep_md_entry->getOperand(1));
		std::set<unsigned> dependencies;
		for(auto j = 0; j<dep_md->getNumOperands(); j++) {
			auto dep_nr = get_val(dep_md->getOperand(j));
			dependencies.insert(dep_nr);
			LLVM_DEBUG(
				dbgs() << dep_nr << "<"; dep_md->getOperand(j)->printAsOperand(dbgs(), F.getParent());
				dbgs() << ">, ";
			);
		}
		LLVM_DEBUG( dbgs() << "\n");
		result[class_nr] = dependencies;
	}

	return result;
}

static const char* EQUIVALENCE_CLASS_INSTRUCTION_MD_NAME = "patmos.eq.class";

void EquivalenceClasses::addClassMetaData(MachineInstr* MI, unsigned class_nr) {
	auto MF = MI->getParent()->getParent();
	auto &C = MF->getFunction().getContext();

	MI->addOperand(*MF, MachineOperand::CreateMetadata(MDNode::get(C, MDString::get(C, EQUIVALENCE_CLASS_INSTRUCTION_MD_NAME))));
	MI->addOperand(*MF, MachineOperand::CreateMetadata(unsigned_md(class_nr, C)));
}

std::optional<unsigned> EquivalenceClasses::getEqClassNr(const MachineInstr* MI) {
	if(!MI->isPredicable() ||
		MI->getOperand(MI->findFirstPredOperandIdx()).getReg() == Patmos::P0
	){
		// Ignore class if instruction is unpredicated
		return std::nullopt;
	}

	std::optional<unsigned> idx;
	for(int i = 0; i<MI->getNumOperands(); i++){
		if(MI->getOperand(i).isMetadata()){
			auto &md = MI->getOperand(i).getMetadata()->getOperand(0);
			if (auto string = dyn_cast<MDString>(md)) {
				if (string->getString() == EQUIVALENCE_CLASS_INSTRUCTION_MD_NAME) {
					idx = i + 1;
					break;
				}
			}
		}
	}
	if(idx) {
		assert(MI->getOperand(*idx).isMetadata());
		return cast<ConstantInt>(dyn_cast<ConstantAsMetadata>(dyn_cast<MDNode>(MI->getOperand(*idx).getMetadata())->getOperand(0))->getValue())->getSExtValue();
	} else {
		return std::nullopt;
	}
}

bool EquivalenceClasses::dependentInstructions(
 		const MachineInstr* instr1,const MachineInstr* instr2,
 		std::map<unsigned, std::set<unsigned> > &class_dependencies
){
	auto predicate_negated = [](const MachineInstr* instr){
		if(instr->isPredicable()) {
			return instr->getOperand(instr->findFirstPredOperandIdx()+1).getImm() == true;
		} else {
			return false;
		}
	};

	auto class1 = EquivalenceClasses::getEqClassNr(instr1);
	auto class2 = EquivalenceClasses::getEqClassNr(instr2);
	if(class1 && class2) {
		if(*class1 == *class2) {
			auto neg1 = predicate_negated(instr1);
			auto neg2 = predicate_negated(instr2);

			// Using the same equivalence class they may be independent if one
			// predicate negation flag is enabled and the other disabled.
			return neg1 == neg2;
		} else {
			auto deps1 = class_dependencies[*class1];
			return class_dependencies[*class1].count(*class2) ||
				// If they aren't dependent on each other's classes, they are still
				// dependent if one or both are negated
				(predicate_negated(instr1) || predicate_negated(instr2));
		}
	} else {
		// Unpredicated instructions are always enabled and so may be enabled with all others
		return true;
	}
}

