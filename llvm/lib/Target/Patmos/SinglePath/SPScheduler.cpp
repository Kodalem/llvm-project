//===---------- SchedulePostRAList.cpp - Scheduler ------------------------===//
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

#include "SPScheduler.h"
#include "SPListScheduler.h"
#include "EquivalenceClasses.h"
#include "TargetInfo/PatmosTargetInfo.h"
#include "llvm/Support/Debug.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/MachineInstr.h"
#include <sstream>


using namespace llvm;

static cl::opt<bool> DisableSPScheduler(
    "mpatmos-disable-singlepath-scheduler",
    cl::init(false),
    cl::desc("Disables the single-path-specific instruction scheduler. (reverts to simple Nop-padding)"),
    cl::Hidden);

static cl::opt<std::string> SPSchedulerConfig(
    "mpatmos-singlepath-scheduler-config",
    cl::init(""),
    cl::desc("Configures the single-path scheduler (function-name,block-number,ignore-first-instructions-count,instructions-to-schedule-count). If block is -1, disables scheduling for all blocks in the function."),
    cl::Hidden);

static cl::opt<bool> SPDisableSchedulerEqClass(
    "mpatmos-disable-singlepath-scheduler-equivalence-class",
    cl::init(false),
    cl::desc("Enables the Single-path scheduler to disregard dependencies between instructions of independent equivalence classes."),
    cl::Hidden);

STATISTIC(SPInstructions,     "Number of instruction bundles in single-path code (both single and double)");
STATISTIC(SPLongInstructions,     "Number of instruction in single-path code that are long");
STATISTIC(SPFirstSlotInstructions,     "Number of instruction in single-path code that can only use the first issue slot (counted before scheduling)");
STATISTIC(SPUnscheduledSlots,     "Number of instruction bundles the single-path scheduler assigned no instructions (used a nop instead)");

char SPScheduler::ID = 0;

FunctionPass *llvm::createSPSchedulerPass(const PatmosTargetMachine &tm) {
  return new SPScheduler(tm);
}

static std::tuple<std::string,int,unsigned,unsigned> split_SPSchedulerConfig()
{
	std::stringstream stream(SPSchedulerConfig.getValue());

	std::string fn_name = "";
	int block_nr = -1;
	unsigned ignore_count = 0;
	unsigned sched_count = 0;

	int i = 0;
	while (stream.good()) {
		std::string substr;
		std::getline(stream, substr, ',');

		switch(i){
		case 0:  fn_name = substr; break;
		case 1:  block_nr = std::stoi(substr); break;
		case 2:  ignore_count = std::stoul(substr); break;
		case 3:  sched_count = std::stoul(substr); break;
		default: assert(false);
		}
		i++;
	}

	return std::make_tuple(fn_name, block_nr, ignore_count, sched_count);
}

static bool config_schedule_for_fn(const MachineFunction *mf) {
	auto fn_name = std::get<0>(split_SPSchedulerConfig());
	return  fn_name != "" && fn_name == mf->getName();
}

static bool config_schedule_for_block(const MachineBasicBlock * mbb) {
	auto block_nr = std::get<1>(split_SPSchedulerConfig());
	return config_schedule_for_fn(mbb->getParent()) && (block_nr == -1 || block_nr == mbb->getNumber());
}

static unsigned config_schedule_ignore_first_count(const MachineBasicBlock * mbb) {
	auto ignore_first_count = std::get<2>(split_SPSchedulerConfig());
	return config_schedule_for_block(mbb)? ignore_first_count: 0;
}

static unsigned config_schedule_instructions_to_schedule_count(const MachineBasicBlock * mbb) {
	auto instruction_schedule_count = std::get<3>(split_SPSchedulerConfig());
	return config_schedule_for_block(mbb)? instruction_schedule_count: 0;
}

static bool disable_schedule_for_fn(const MachineFunction *mf) {
	auto block_nr = std::get<1>(split_SPSchedulerConfig());
	return config_schedule_for_fn(mf) && block_nr == -1;
}

static bool disable_schedule_for_block(const MachineBasicBlock * mbb) {
	auto ignore_first_count = std::get<2>(split_SPSchedulerConfig());
	auto instruction_schedule_count = std::get<3>(split_SPSchedulerConfig());
	return
		config_schedule_for_fn(mbb->getParent()) &&
		config_schedule_for_block(mbb) &&
		ignore_first_count == 0 &&
		instruction_schedule_count == 0;
}

/////////////////////////////////////////////
// Functions for use with the list scheduler
/////////////////////////////////////////////

/// If set contains Patmos::S0, replaces it with Patmos::P0-Patmos::P7.
///
/// This is needed because S0 contains the others but using '==' doesn't
/// show this (i.e. S0 != P1).
void convert_s0_to_p(std::set<Register> &regs) {
  if(regs.count(Patmos::S0)){
    regs.erase(Patmos::S0);
    std::set<Register> predicate_regs = {
        Patmos::P0,Patmos::P1,Patmos::P2,Patmos::P3,
        Patmos::P4,Patmos::P5,Patmos::P6,Patmos::P7};
    regs.insert(predicate_regs.begin(), predicate_regs.end());
  }
}

std::set<Register> reads(const MachineInstr *instr){
  std::set<Register> result;
  auto opcode = instr->getOpcode();
  std::for_each(instr->operands_begin(), instr->operands_end(), [&](auto op){
    if(op.isReg() && op.isUse()) {
      result.insert(op.getReg());
    }
  });

  // single-path functions take p7 as the predicate that enables/disables them.
  // This is not encoded in the def/use list, so add it here.
  // This is a workaround. Optimally, the list of implicits would include P7
  // but could not be made to work.
  if(instr->isCall()) {
    result.insert(Patmos::P7);
  }

  if(isStackInst(opcode) && !isStackMgmtInst(opcode)) {
    result.insert(Patmos::SS);
    result.insert(Patmos::ST);
  }

  convert_s0_to_p(result);
  return result;
}

std::set<Register> writes(const MachineInstr *instr){
  std::set<Register> result;
  auto opcode = instr->getOpcode();
  std::for_each(instr->operands_begin(), instr->operands_end(), [&](auto op){
    if(op.isReg() && op.isDef()) {
      result.insert(op.getReg());
    }
  });
  convert_s0_to_p(result);
  if(instr->isCall()) {
    //Special case RSP and RFP
    result.insert(Patmos::RSP);
    result.insert(Patmos::RFP);
  }

  return result;
}

Optional<Register> uses_predicate(const MachineInstr *instr){
  Optional<Register> result;

  if(instr->isPredicable()) {
  	result = instr->getOperand(instr->findFirstPredOperandIdx()).getReg();
  }

  return result;
}

bool poisons(const MachineInstr *instr) {
  return isLoadInst(instr->getOpcode());
}

bool memory_access(const MachineInstr *instr) {
  auto opcode = instr->getOpcode();
  return isLoadInst(opcode) || isStoreInst(opcode) || instr->isCall() || instr->isReturn() || isStackMgmtInst(opcode);
}

unsigned latency(const MachineInstr *instr) {
  auto opcode = instr->getOpcode();
  if(isLoadInst(opcode) || opcode == Patmos::MUL || opcode == Patmos::MULU) {
    return 1;
  } else {
    return 0;
  }
}

bool is_constant(Register r) {
  return r == Patmos::R0 || r == Patmos::P0;
}

bool conditional_branch(const MachineInstr *instr) {
  auto opcode = instr->getOpcode();
  switch(opcode){
  case Patmos::BR:
  case Patmos::BRCF:
  case Patmos::BRCFND:
  case Patmos::BRCFNDu:
  case Patmos::BRCFR:
  case Patmos::BRCFRND:
  case Patmos::BRCFRNDu:
  case Patmos::BRCFRu:
  case Patmos::BRCFT:
  case Patmos::BRCFTND:
  case Patmos::BRCFTNDu:
  case Patmos::BRCFTu:
  case Patmos::BRCFu:
  case Patmos::BRND:
  case Patmos::BRNDu:
  case Patmos::BRR:
  case Patmos::BRRND:
  case Patmos::BRRNDu:
  case Patmos::BRRu:
  case Patmos::BRT:
  case Patmos::BRTND:
  case Patmos::BRTNDu:
  case Patmos::BRTu:
  case Patmos::BRu:
    return true;
  default:
    return instr->isReturn();
  }
}

bool is_long(const MachineInstr *instr) {
  if(instr->isInlineAsm() || instr->isPseudo()) {
    // We don't want them to affect the scheduling,
    // so act as if they are long
    return true;
  }
  switch(instr->getDesc().getSize()) {
  case 4: return false;
  case 8: return true;
  default:
    report_fatal_error("Failed because of unexpected instruction size");
  }
}

bool may_second_slot(const PatmosInstrInfo *TII, const MachineInstr *instr) {
  auto opcode = instr->getOpcode();
  if(instr->isInlineAsm() || instr->isPseudo()) {
    // We don't want them to affect the scheduling,
    // so act as if they can't be in second slot
    return false;
  }
  if(PatmosSubtarget::usePermissiveDualIssue() && (
    isLoadInst(opcode) || isStoreInst(opcode) || isControlFlowInst(opcode) || isMultiplyInst(opcode)
  )){
    return true;
  } else {
    return TII->canIssueInSlot(instr, 1);
  }
}

/// At this point in the single-path transformation, all branches use
/// the Patmos::BR opcode. Later, some 'BR's might turn into 'BRCF'
/// depending on block sizes and how the function spiller splits.
/// Therefore, at this stage all branches must be assumed to be BRCF
/// and as such touch main memory.
static bool sp_scheduler_is_main_mem_instr(unsigned opcode) {
	return isMainMemInst(opcode) || (opcode == Patmos::BR);
}

bool may_bundle(const MachineInstr *instr1, const MachineInstr *instr2) {
  auto opcode1 = instr1->getOpcode();
  auto opcode2 = instr2->getOpcode();

  auto is_combination = [&](bool(*req1)(unsigned), bool(*req2)(unsigned)) {
    return (req1(opcode1) && req2(opcode2)) || (req1(opcode2) && req2(opcode1));
  };

  if(PatmosSubtarget::usePermissiveDualIssue() && (
    is_combination(isLoadInst, isLoadInst) ||
	is_combination(isStoreInst, isStoreInst) ||
	is_combination(isLoadInst, isStoreInst) ||
	is_combination(isStackInst, isStackInst) ||
	is_combination(sp_scheduler_is_main_mem_instr, sp_scheduler_is_main_mem_instr) ||
	is_combination(isControlFlowInst, isControlFlowInst) ||
	is_combination(isMultiplyInst, isMultiplyInst)
  )) {
    return false;
  }
  return true;
}

/////////////////////////////////////////////
// End of functions for use with the list scheduler
/////////////////////////////////////////////

SPScheduler::SPScheduler(const PatmosTargetMachine &tm):
    MachineFunctionPass(ID), TM(tm)
{
  auto warning = "Warning: '--";
  auto post_msg = "' flag is meant for internal compiler development only and shouldn't be used otherwise.\n";
  if(DisableSPScheduler) {
    errs() << warning << "mpatmos-disable-singlepath-scheduler" << post_msg;
  }
  if(std::get<0>(split_SPSchedulerConfig()) != "") {
    errs() << warning << "mpatmos-singlepath-scheduler-config" << post_msg;
  }
}

bool SPScheduler::runOnMachineFunction(MachineFunction &mf){


  // Only schedule single-path function
  if(! mf.getInfo<PatmosMachineFunctionInfo>()->isSinglePath() ){
    return false;
  }

  LLVM_DEBUG( dbgs() << "Running SPScheduler on function '" <<  mf.getName() << "'\n");

  for(auto &mbb: mf){
    LLVM_DEBUG( dbgs() << "MBB before scheduling: \n"; mbb.dump());

    // Statistics
    for(auto instrIter = mbb.begin(), instrEnd = mbb.end();
            instrIter != instrEnd; instrIter++)
	{
    	if(is_long(&*instrIter)) SPLongInstructions++;
    	if(!(instrIter->isInlineAsm() || instrIter->isPseudo()) && !may_second_slot(TM.getSubtargetImpl()->getInstrInfo(), &*instrIter)) SPFirstSlotInstructions++;
	}

    bool disable = DisableSPScheduler || disable_schedule_for_fn(&mf) || disable_schedule_for_block(&mbb);

    if(!disable) {
      runListSchedule(&mbb);
    } else {
      LLVM_DEBUG( dbgs() << "Disabled SPScheduler for '" << mf.getName() << "' block: " << mbb.getNumber() << "\n");
    }

    for(auto instrIter = mbb.begin(), instrEnd = mbb.end();
        instrIter != instrEnd; )
    {
      SPInstructions++;
      auto latency = calculateLatency(instrIter, disable);
	  for(auto i = 0; i<latency; i++){
	    SPInstructions++;
	    TM.getInstrInfo()->insertNoop(mbb, std::next(instrIter));
	  }
	  instrIter = std::next(instrIter, 1+latency); // Make sure to skip the newly added noops
    }
  }

  // Look for any fall-through that has a load at the end and ensure
  // that nops are added if the target uses the load destination
  //
  // We do this here, instead of as part of the list scheduling, because it is
  // quite a rare occurrence and likely not critical to performance.
  for(auto &mbb: mf) {
    if(mbb.canFallThrough()) {
	  auto fall_to = mbb.getFallThrough();
	  assert(fall_to);
	  bool need_nop = false;

	  auto check_instr = [&](MachineInstr *may_load){
		  if(isLoadInst(may_load->getOpcode())){
			  auto dest = may_load->getOperand(0).getReg();

			  if(reads(&*fall_to->begin()).count(dest) ||
				  (fall_to->begin()->isBundledWithSucc() &&
					reads(&*std::next(fall_to->instr_begin())).count(dest))
			  ) {
				  need_nop = true;
			  }
		  }
	  };

	  if(std::prev(mbb.end())->isBundledWithSucc()) {
		  // Check both instruction in last bundle
		  check_instr(&*std::prev(mbb.instr_end()));
	  }
	  check_instr(&*std::prev(mbb.end()));

	  if(need_nop) {
		  SPInstructions++;
		  TM.getInstrInfo()->insertNoop(mbb, mbb.end());
		  LLVM_DEBUG(
			dbgs() << "Added load delay to end of bb." << mbb.getNumber() << "." << mbb.getName()
				<< " because successor bb." << fall_to->getNumber() << "." << fall_to->getName() << " uses load destination\n";
		  );
	  }
	}
  }

  return true;
}

unsigned SPScheduler::calculateLatency(MachineBasicBlock::iterator instr, bool branches_only) const{
  if((instr->isBranch() || instr->isCall() || instr->isReturn()) && instr->hasDelaySlot()){
    // We simply add 3 nops after any branch, call or return, as its
    // the highest possible delay.
    return 3;
  } else if (branches_only &&
      (instr->mayLoad() || (instr->getOpcode() == Patmos::MUL) || (instr->getOpcode() == Patmos::MULU))
  ){
    return 1;
  } else {
    return 0;
  }
}

void SPScheduler::runListSchedule(MachineBasicBlock *mbb) {
  // Scheduler cannot handle the PSEUDO_LOOPBOUND pseudo-instruction,
  // so if it's there, move it to the end of the instruction list
  // so its skipped
  auto found_loopbound = std::find_if(mbb->instr_begin(), mbb->instr_end(), [&](auto &instr) {
    return instr.getOpcode() == Patmos::PSEUDO_LOOPBOUND;
  });
  bool was_loopbound = false;
  if (found_loopbound != mbb->instr_end()) {
    auto *loopbound_instr = &*found_loopbound;
    mbb->remove_instr(loopbound_instr);
    mbb->insert(mbb->instr_end(), loopbound_instr);
    was_loopbound = true;
  }
  auto last_instr = [&](){
    auto result = mbb->instr_end();
    if(was_loopbound) {
      result--;
    }
    return result;
  };

  unsigned total_instr_count = std::distance(mbb->instr_begin(), last_instr());

  unsigned ignore_count;
  if(config_schedule_ignore_first_count(mbb) != 0) {
	  ignore_count = config_schedule_ignore_first_count(mbb);
  } else {
	  ignore_count = 0;
  }

  // If requested, don't schedule the first X instruction (move them to the end immediately
  for(auto i = 0; i<std::min(total_instr_count, ignore_count); i++) {
      // Add nop to ensure any need delays after e.g. loads are adhered to
      TM.getInstrInfo()->insertNoop(*mbb, last_instr());
	  auto instr = &*mbb->instr_begin();
	  mbb->remove_instr(instr);
      mbb->insert(last_instr(), instr);
  }
  if(ignore_count<total_instr_count && ignore_count>0){
	  // Add some nops to easily show where the divide between scheduled and unscheduled is
      TM.getInstrInfo()->insertNoop(*mbb, last_instr());
      TM.getInstrInfo()->insertNoop(*mbb, last_instr());
      TM.getInstrInfo()->insertNoop(*mbb, last_instr());
      TM.getInstrInfo()->insertNoop(*mbb, last_instr());
      TM.getInstrInfo()->insertNoop(*mbb, last_instr());
  }
  auto schedule_count = total_instr_count-ignore_count;
  if(config_schedule_instructions_to_schedule_count(mbb) != 0) {
	  schedule_count = std::min(schedule_count, config_schedule_instructions_to_schedule_count(mbb));
  }

  auto last_to_schedule = std::next(mbb->instr_begin(),schedule_count);

  llvm::Optional<std::tuple<
    const PatmosInstrInfo *,
    bool (*)(const PatmosInstrInfo *, const MachineInstr *),
    bool (*)(const MachineInstr *),
    bool (*)(const MachineInstr *,const MachineInstr *)
  >> enable_dual_issue;

  if(PatmosSubtarget::enableBundling()) {
    // Enable dual-issue
    enable_dual_issue = std::make_tuple(TM.getSubtargetImpl()->getInstrInfo(), may_second_slot, is_long, may_bundle);
  } else {
    // disable dual-issue
    enable_dual_issue = None;
  }


  auto is_dependent = [&](const MachineInstr* instr1,const MachineInstr* instr2){
    bool dep;
    if(!PatmosSinglePathInfo::useNewSinglePathTransform() || SPDisableSchedulerEqClass)  {
    	dep = true;
    } else {
    	auto class_dependencies = EquivalenceClasses::importClassDependenciesFromModule(*mbb->getParent());
    	dep = EquivalenceClasses::dependentInstructions(instr1, instr2,class_dependencies);
    }
	LLVM_DEBUG(
		dbgs() << "Checking Instructions dependence:\n";
		instr1->dump();
		instr2->dump();
		dbgs() << "Dependent: " << dep << "\n";
	);
	return dep;
  };

  auto schedule = list_schedule(
    mbb->instr_begin(), last_to_schedule,
    reads, writes, uses_predicate, poisons, memory_access, latency, is_constant, conditional_branch, enable_dual_issue,
	is_dependent
  );
  LLVM_DEBUG(
    dbgs() << "List Schedule (New Order <- old order):\n";
    for(auto entry: schedule) {
      dbgs() << entry.first << "<-" << entry.second << "\n";
    }
  );
  if(schedule.size() == 0)  {
    LLVM_DEBUG( dbgs() << "No Scheduling done\n");
    return;
  }

  auto last_new_idx = schedule.rbegin()->first;

  // Get a map from new index to the instruction that should be in it
  std::map<unsigned, MachineInstr*> instr_map;
  for(auto entry: schedule) {
    instr_map[entry.first] = &(*std::next(mbb->instr_begin(), entry.second));
  }

  // We rebuild the schedule by extracting each instruction needed at a given index
  // and inserting it at the end of the list in the new order.
  for(auto idx = 0; idx <= last_new_idx; idx++) {
    if(instr_map.count(idx)) {
      auto *instr = instr_map[idx];
      mbb->remove_instr(instr);
      mbb->insert(last_instr(), instr);
      if(enable_dual_issue && (idx%2!=0)) {
        // Instruction is issued in second issue slot,
        // Bundle it with the first slot
        instr->bundleWithPred();
      }
    } else if(enable_dual_issue && (idx%2!=0)) {
      // There is no instruction scheduled in the second issue slot
    }else{
      // Nop is needed in at this index (first issue slot)
      TM.getInstrInfo()->insertNoop(*mbb, last_instr());
      SPUnscheduledSlots++;
    }
  }

  auto max_possible_moved = config_schedule_instructions_to_schedule_count(mbb) == 0?
	  total_instr_count :
	  std::min(total_instr_count,config_schedule_ignore_first_count(mbb) + config_schedule_instructions_to_schedule_count(mbb));
  auto unschedule_at_end = total_instr_count - max_possible_moved;
  if(unschedule_at_end>0){
	  // Add some nops to easily show where the divide between scheduled and unscheduled is
      TM.getInstrInfo()->insertNoop(*mbb, last_instr());
      TM.getInstrInfo()->insertNoop(*mbb, last_instr());
      TM.getInstrInfo()->insertNoop(*mbb, last_instr());
      TM.getInstrInfo()->insertNoop(*mbb, last_instr());
      TM.getInstrInfo()->insertNoop(*mbb, last_instr());
  }
  // Move any unscheduled instruction to the end
  for(auto i = 0; i<unschedule_at_end; i++){
      // Add nop to ensure any need delays after e.g. loads are adhered to
      TM.getInstrInfo()->insertNoop(*mbb, last_instr());
	  auto instr = &*mbb->instr_begin();
	  mbb->remove_instr(instr);
      mbb->insert(last_instr(), instr);
  }
  LLVM_DEBUG( dbgs() << "MBB after list scheduling: \n"; mbb->dump());
}




