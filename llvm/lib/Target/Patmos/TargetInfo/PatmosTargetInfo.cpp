//===-- PatmosTargetInfo.cpp - Patmos Target Implementation ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Patmos.h"
#include "PatmosTargetInfo.h"
#include "PatmosRegisterInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Compiler.h"
#include "llvm/CodeGen/MachineFunction.h"
#include <optional> // https://llvm.org/docs/CodingStandards.html
#include "llvm/CodeGen/MachineModuleInfo.h"

using namespace llvm;


void llvm::getMBBIRName(const MachineBasicBlock *MBB,
                         SmallString<128> &result) {
  const BasicBlock *BB = MBB->getBasicBlock();
  const Twine bbname = (BB && BB->hasName()) ? BB->getName()
                                             : "<anonymous>";
  const Twine bb_ir_label = "#" + MBB->getParent()->getFunction().getName() +
                            "#" + bbname +
                            "#" + Twine(MBB->getNumber());
  bb_ir_label.toVector(result);
}

std::optional<std::pair<uint64_t, uint64_t>> llvm::getLoopBounds(const MachineBasicBlock * MBB) {
  if(MBB && MBB->getBasicBlock()) {
    auto bound_instr = std::find_if(MBB->begin(), MBB->end(), [&](auto &instr){
      return instr.getOpcode() == Patmos::PSEUDO_LOOPBOUND;
    });
    if(bound_instr != MBB->end()) {
      assert(bound_instr->getOperand(0).isImm());
      assert(bound_instr->getOperand(1).isImm());
      assert(bound_instr->getOperand(0).isImm() >= 0);
      assert(bound_instr->getOperand(1).isImm() >= 0);

      auto min = bound_instr->getOperand(0).getImm() + 1;
      auto max = min + bound_instr->getOperand(1).getImm();

      // Check for overflows
      if(min < 0 || max < min) {
        report_fatal_error("Invalid loop bounds");
      }

      return std::make_pair((uint64_t) min, (uint64_t) max);
    }
  }
  return std::nullopt;
}

Target &llvm::getThePatmosTarget() {
  static Target ThePatmosTarget;
  return ThePatmosTarget;
}

const Function *llvm::getCallTarget(const MachineInstr *MI) {
  const Module *M = MI->getParent()->getParent()->getFunction().getParent();
  const MachineOperand &MO = MI->getOperand(2);
  const Function *Target = NULL;
  llvm::StringRef TargetName = "";
  if (MO.isGlobal()) {
    Target = dyn_cast<Function>(MO.getGlobal());
    if(!Target) {
      TargetName = MO.getGlobal()->getName();
    }
  } else if (MO.isSymbol()) {
    TargetName = MO.getSymbolName();
    Target = M->getFunction(TargetName);
  }

  if(!Target) {
    if(auto *alias = M->getNamedAlias(TargetName)) {
      Target = M->getFunction(alias->getAliasee()->getName());
    }
  }

  return Target;
}


MachineFunction *llvm::getCallTargetMF(const MachineInstr *MI, MachineModuleInfo &MMI) {
  auto *F = getCallTarget(MI);
  if (F) {
    // getOrCreateMachineFunction is now accessed via the passed MMI reference
    return &MMI.getOrCreateMachineFunction(const_cast<Function&>(*F));
  }
  return nullptr;
}

bool llvm::isLoadInst(unsigned opcode) {
  switch( opcode ){
    default:
      return isMainMemLoadInst(opcode);
    case Patmos::LBL:
    case Patmos::LBS:
    case Patmos::LBUL:
    case Patmos::LBUS:
    case Patmos::LHL:
    case Patmos::LHS:
    case Patmos::LHUL:
    case Patmos::LHUS:
    case Patmos::LWL:
    case Patmos::LWS:
      return true;
  }
}

bool llvm::isMainMemLoadInst(unsigned opcode) {
  switch( opcode ){
    default:
      return false;
    case Patmos::LBC:
    case Patmos::LBM:
    case Patmos::LBUC:
    case Patmos::LBUM:
    case Patmos::LHC:
    case Patmos::LHM:
    case Patmos::LHUC:
    case Patmos::LHUM:
    case Patmos::LWC:
    case Patmos::LWM:
      return true;
  }
}

bool llvm::isStoreInst(unsigned opcode) {
  switch( opcode ){
    default:
      return isMainMemStoreInst(opcode);
    case Patmos::SBL:
    case Patmos::SBS:
    case Patmos::SHL:
    case Patmos::SHS:
    case Patmos::SWL:
    case Patmos::SWS:
      return true;
  }
}

bool llvm::isMainMemStoreInst(unsigned opcode) {
  switch( opcode ){
    default:
      return false;
    case Patmos::SBC:
    case Patmos::SBM:
    case Patmos::SHC:
    case Patmos::SHM:
    case Patmos::SWC:
    case Patmos::SWM:
      return true;
  }
}

bool llvm::isStackMgmtInst(unsigned opcode) {
  switch( opcode ){
    default:
      return false;
    case Patmos::SRESi:
    case Patmos::SENSi:
    case Patmos::SENSr:
    case Patmos::SFREEi:
    case Patmos::SSPILLi:
    case Patmos::SSPILLr:
      return true;
  }
}

bool llvm::isStackInst(unsigned opcode) {
  switch( opcode ){
    default:
      return isStackMgmtInst(opcode);
    case Patmos::LBS:
    case Patmos::LBUS:
    case Patmos::LHS:
    case Patmos::LHUS:
    case Patmos::LWS:
    case Patmos::SBS:
    case Patmos::SHS:
    case Patmos::SWS:
      return true;
  }
}

bool llvm::isControlFlowInst(unsigned opcode) {
  return isControlFlowCFInst(opcode) || isControlFlowNonCFInst(opcode);
}

bool llvm::isControlFlowNonCFInst(unsigned opcode) {
  switch( opcode ){
    default:
      return false;
    case Patmos::BR:
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
  }
}

bool llvm::isControlFlowCFInst(unsigned opcode) {
  switch( opcode ){
    default:
      return false;
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
    case Patmos::CALL:
    case Patmos::CALLND:
    case Patmos::CALLR:
    case Patmos::CALLRND:
    case Patmos::RET:
    case Patmos::RETND:
    // TODO: traps, interrupts
      return true;
  }
}

bool llvm::isMultiplyInst(unsigned opcode) {
  return opcode == Patmos::MUL || opcode == Patmos::MULU;
}


bool llvm::isMainMemInst(unsigned opcode) {
  return isMainMemLoadInst(opcode) || isMainMemStoreInst(opcode) || isControlFlowCFInst(opcode) || isStackMgmtInst(opcode);
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializePatmosTargetInfo() {
  RegisterTarget<Triple::patmos> X(getThePatmosTarget(), "patmos",
                                   "Patmos [experimental]", "Patmos");
}
