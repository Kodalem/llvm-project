//======-- PatmosFrameLowering.cpp - Patmos Frame Information -------=========//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the Patmos implementation of TargetFrameLowering class.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "patmos-framelowering"
#include "PatmosFrameLowering.h"
#include "PatmosInstrInfo.h"
#include "PatmosMachineFunctionInfo.h"
#include "SinglePath/PatmosSinglePathInfo.h"
#include "PatmosSubtarget.h"
#include "PatmosTargetMachine.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/IR/Attributes.h"

#include "PatmosGenRegisterInfo.inc"

using namespace llvm;

namespace llvm {
  /// Count the number of FIs overflowing into the shadow stack
  STATISTIC(FIsNotFitSC, "FIs that did not fit in the stack cache");
}

/// DisableStackCache - Command line option to disable the usage of the stack 
/// cache (enabled by default).
static cl::opt<bool> DisableStackCache("mpatmos-disable-stack-cache",
                            cl::init(false),
                            cl::desc("Disable the use of Patmos' stack cache"));

/// EnableBlockAlignedStackCache - Command line option to enable the usage of 
/// the block-aligned stack cache (disabled by default).
static cl::opt<bool> EnableBlockAlignedStackCache
          ("mpatmos-enable-block-aligned-stack-cache", cl::init(false),
           cl::desc("Enable the use of Patmos' block-aligned stack cache"));

bool PatmosFrameLowering::hasFPImpl(const MachineFunction &MF) const {
  const MachineFrameInfo &MFI = MF.getFrameInfo();

  // Naked functions should not use the stack, they do not get a frame pointer.
  if (MF.getFunction().hasFnAttribute("naked"))
    return false;

  return (MF.getTarget().Options.DisableFramePointerElim(MF) ||
          MF.getFrameInfo().hasVarSizedObjects() ||
          MFI.isFrameAddressTaken());
}

static unsigned int align(unsigned int offset, unsigned int alignment) {
  return ((offset + alignment - 1) / alignment) * alignment;
}

unsigned PatmosFrameLowering::getEffectiveStackCacheSize() const
{
  return EnableBlockAlignedStackCache ? 
                        STC.getStackCacheSize() - STC.getStackCacheBlockSize() : 
                        STC.getStackCacheSize();
}

unsigned PatmosFrameLowering::getEffectiveStackCacheBlockSize() const
{
  return EnableBlockAlignedStackCache ? 4 : STC.getStackCacheBlockSize();
}

unsigned PatmosFrameLowering::getAlignedStackCacheFrameSize(
                                                       unsigned frameSize) const
{
  if (frameSize == 0)
    return frameSize;
  else
    return EnableBlockAlignedStackCache ? frameSize : 
                                        STC.getAlignedStackFrameSize(frameSize);
}

void PatmosFrameLowering::assignFIsToStackCache(MachineFunction &MF,
                                                BitVector &SCFIs) const
{
  MachineFrameInfo &MFI = MF.getFrameInfo();
  PatmosMachineFunctionInfo &PMFI = *MF.getInfo<PatmosMachineFunctionInfo>();
  const std::vector<CalleeSavedInfo> &CSI = MFI.getCalleeSavedInfo();
  const TargetRegisterInfo *TRI = STC.getRegisterInfo();

  assert(MFI.isCalleeSavedInfoValid());

  // find all FIs used for callee saved registers
  for(std::vector<CalleeSavedInfo>::const_iterator i(CSI.begin()),
      ie(CSI.end()); i != ie; i++)
  {
    if (i->getReg() == Patmos::S0 && PMFI.getS0SpillReg()) continue;
    // Predicates are handled via aliasing to S0. They appear here when we
    // skip assigning s0 to a stack slot, not really sure why.
    if (TRI->getRegClass(Patmos::PRegsRegClassID)->contains(i->getReg())) continue;
    SCFIs[i->getFrameIdx()] = true;
  }

  // RegScavenging register
  if (TRI->requiresRegisterScavenging(MF)) {
    SCFIs[PMFI.getRegScavengingFI()] = true;
  }

  // Spill slots / storage introduced for single path conversion
  const std::vector<int> &SinglePathFIs = PMFI.getSinglePathFIs();
  for(unsigned i=0; i<SinglePathFIs.size(); i++) {
    SCFIs[SinglePathFIs[i]] = true;
  }

  // find all FIs that are spill slots
  for(unsigned FI = 0, FIe = MFI.getObjectIndexEnd(); FI != FIe; FI++) {
    if (MFI.isDeadObjectIndex(FI))
      continue;

    // find all spill slots and locations for callee saved registers
    if (MFI.isSpillSlotObjectIndex(FI))
      SCFIs[FI] = true;
  }
}



unsigned PatmosFrameLowering::assignFrameObjects(MachineFunction &MF,
                                                 bool UseStackCache) const
{
  MachineFrameInfo &MFI = MF.getFrameInfo();
  PatmosMachineFunctionInfo &PMFI = *MF.getInfo<PatmosMachineFunctionInfo>();
  unsigned maxFrameSize = MFI.getMaxCallFrameSize();

  // defaults to false (all objects are assigned to shadow stack)
  BitVector SCFIs(MFI.getObjectIndexEnd());

  if (UseStackCache) {
    assignFIsToStackCache(MF, SCFIs);
  }

  // assign new offsets to FIs

  // next stack slot in stack cache
  unsigned int SCOffset = 0;
  // next stack slot in shadow stack
  // Also reserve space for the call frame if we do not use a frame pointer.
  // This must be in sync with PatmosRegisterInfo::eliminateCallFramePseudoInstr
  unsigned int SSOffset = (hasFP(MF) ? 0 : maxFrameSize);

  LLVM_DEBUG(dbgs() << "PatmosSC: " << MF.getFunction().getName() << "\n");
  LLVM_DEBUG(MFI.print(MF, dbgs()));
  for(unsigned FI = 0, FIe = MFI.getObjectIndexEnd(); FI != FIe; FI++) {
    if (MFI.isDeadObjectIndex(FI))
      continue;

    // MachineFrameInfo::getObjectAlign returns an Align object in modern LLVM.
    Align FIAlign = MFI.getObjectAlign(FI);
    unsigned FIalignment = FIAlign.value();
    int64_t FIsize = MFI.getObjectSize(FI);

    if (FIsize > INT_MAX) {
      report_fatal_error("Frame objects with size > INT_MAX not supported.");
    }

    // be sure to catch some special stack objects not expected for Patmos
    assert(!MFI.isFixedObjectIndex(FI) && !MFI.isObjectPreAllocated(FI));

    // assigned to stack cache or shadow stack?
    if (SCFIs[FI]) {
      // alignment
      unsigned int next_SCOffset = align(SCOffset, FIalignment);

      // check if the FI still fits into the SC
      if (align(next_SCOffset + FIsize, getEffectiveStackCacheBlockSize()) <=
          getEffectiveStackCacheSize()) {
        LLVM_DEBUG(dbgs() << "PatmosSC: FI: " << FI << " on SC: " << next_SCOffset
                    << "(" << MFI.getObjectOffset(FI) << ")\n");

        // reassign stack offset
        MFI.setObjectOffset(FI, next_SCOffset);

        // reserve space on the stack cache
        SCOffset = next_SCOffset + FIsize;

        // the FI is assigned to the SC, process next FI
        continue;
      }
      else {
        // the FI did not fit in the SC -- fall-through and put it on the 
        // shadow stack
        SCFIs[FI] = false;
        FIsNotFitSC++;
      }
    }

    // assign the FI to the shadow stack
    {
      assert(!SCFIs[FI]);

      // alignment
      SSOffset = align(SSOffset, FIalignment);

      LLVM_DEBUG(dbgs() << "PatmosSC: FI: " << FI << " on SS: " << SSOffset
                   << "(" << MFI.getObjectOffset(FI) << ")\n");

      // reassign stack offset
      MFI.setObjectOffset(FI, SSOffset);

      // reserve space on the shadow stack
      SSOffset += FIsize;
    }
  }

  // align stack frame on stack cache
  unsigned stackCacheSize = align(SCOffset, getEffectiveStackCacheBlockSize());

  assert(stackCacheSize <= getEffectiveStackCacheSize());

  // align shadow stack. call arguments are already included in SSOffset
  unsigned stackSize = align(SSOffset, getStackAlignment());

  // update offset of fixed objects
  for(unsigned FI = MFI.getObjectIndexBegin(), FIe = 0; FI != FIe; FI++) {
    // reassign stack offset
    MFI.setObjectOffset(FI, MFI.getObjectOffset(FI) + stackSize);
  }

  LLVM_DEBUG(MFI.print(MF, dbgs()));

  // store assignment information
  PMFI.setStackCacheReservedBytes(stackCacheSize);
  PMFI.setStackCacheFIs(SCFIs);

  PMFI.setStackReservedBytes(stackSize);
  MFI.setStackSize(stackSize);

  return stackSize;
}



MachineInstr *PatmosFrameLowering::emitSTC(MachineFunction &MF, MachineBasicBlock &MBB,
                                  MachineBasicBlock::iterator &MI,
                                  unsigned Opcode) const {
  PatmosMachineFunctionInfo &PMFI = *MF.getInfo<PatmosMachineFunctionInfo>();

  // align the stack cache frame size
  unsigned alignedStackSize = getAlignedStackCacheFrameSize(
                                     PMFI.getStackCacheReservedBytes());
  assert(alignedStackSize <= getEffectiveStackCacheSize());

  // STC instructions are specified in words
  unsigned stackFrameSize = alignedStackSize / 4;

  MachineInstr *Inst = NULL;

  if (stackFrameSize) {
    assert(isUInt<22>(stackFrameSize) && "Stack cache size exceeded.");

    DebugLoc DL = (MI != MBB.end()) ? MI->getDebugLoc() : DebugLoc();

    const TargetInstrInfo &TII = *STC.getInstrInfo();

    // emit reserve instruction
    Inst = AddDefaultPred(BuildMI(MBB, MI, DL, TII.get(Opcode)))
      .addImm(stackFrameSize);
  }
  return Inst;
}

void PatmosFrameLowering::patchCallSites(MachineFunction &MF) const {
  // visit all basic blocks
  for (MachineFunction::iterator i(MF.begin()), ie(MF.end()); i != ie; ++i) {
    for (MachineBasicBlock::iterator j(i->begin()), je=(i->end()); j != je;
         j++) {
      // a call site?
      if (j->isCall()) {
        MachineBasicBlock::iterator p(std::next(j));
        emitSTC(MF, *i, p, Patmos::SENSi);
      }
    }
  }
}

void PatmosFrameLowering::emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const {
  const TargetInstrInfo *TII = STC.getInstrInfo();

  MachineBasicBlock::iterator MBBI = MBB.begin();
  DebugLoc dl = MBBI != MBB.end() ? MBBI->getDebugLoc() : DebugLoc();

  //----------------------------------------------------------------------------
  // Handle the stack cache -- if enabled.

  // assign some FIs to the stack cache if possible
  unsigned stackSize = assignFrameObjects(MF, !DisableStackCache);

  if (!DisableStackCache) {
    // emit a reserve instruction
    MachineInstr *MI = emitSTC(MF, MBB, MBBI, Patmos::SRESi);
    if (MI) MI->setFlag(MachineInstr::FrameSetup);

    // patch all call sites
    patchCallSites(MF);
  }

  //----------------------------------------------------------------------------
  // Handle the shadow stack

  // Do we need to allocate space on the stack?
  if (stackSize) {
    // adjust stack : sp -= stack size
    if (stackSize <= 0xFFF) {
      MachineInstr *MI = AddDefaultPred(BuildMI(MBB, MBBI, dl,
            TII->get(Patmos::SUBi), Patmos::RSP))
        .addReg(Patmos::RSP).addImm(stackSize);
      MI->setFlag(MachineInstr::FrameSetup);

    }
    else {
      MachineInstr *MI = AddDefaultPred(BuildMI(MBB, MBBI, dl,
            TII->get(Patmos::SUBl), Patmos::RSP))
        .addReg(Patmos::RSP).addImm(stackSize);
      MI->setFlag(MachineInstr::FrameSetup);
    }
  }

}

void PatmosFrameLowering::emitEpilogue(MachineFunction &MF,
                                       MachineBasicBlock &MBB) const {
  MachineBasicBlock::iterator MBBI = MBB.getLastNonDebugInstr();
  MachineFrameInfo &MFI            = MF.getFrameInfo();
  const TargetInstrInfo *TII       = STC.getInstrInfo();
  DebugLoc dl                      = MBBI->getDebugLoc();

  //----------------------------------------------------------------------------
  // Handle Stack Cache

  // emit a free instruction
  MachineInstr *MI = emitSTC(MF, MBB, MBBI, Patmos::SFREEi);
  if (MI) MI->setFlag(MachineInstr::FrameSetup);

  //----------------------------------------------------------------------------
  // Handle Shadow Stack

  // Get the number of bytes from FrameInfo
  unsigned stackSize = MFI.getStackSize();

  // adjust stack  : sp += stack size
  if (stackSize) {
    if (stackSize <= 0xFFF) {
      MachineInstr *MI = AddDefaultPred(BuildMI(MBB, MBBI, dl,
            TII->get(Patmos::ADDi), Patmos::RSP))
        .addReg(Patmos::RSP).addImm(stackSize);
      MI->setFlag(MachineInstr::FrameSetup);
    }
    else {
      MachineInstr *MI = AddDefaultPred(BuildMI(MBB, MBBI, dl,
            TII->get(Patmos::ADDl), Patmos::RSP))
        .addReg(Patmos::RSP).addImm(stackSize);
      MI->setFlag(MachineInstr::FrameSetup);
    }
  }
}

void PatmosFrameLowering::determineCalleeSaves(MachineFunction &MF,
                                               BitVector &SavedRegs,
                                               RegScavenger *RS) const {
  TargetFrameLowering::determineCalleeSaves(MF, SavedRegs, RS);

  const TargetInstrInfo *TII = TM.getInstrInfo();
  const TargetRegisterInfo *TRI = TM.getRegisterInfo();
  MachineFrameInfo &MFI = MF.getFrameInfo();
  PatmosMachineFunctionInfo &PMFI = *MF.getInfo<PatmosMachineFunctionInfo>();

  // Insert instructions at the beginning of the entry block;
  // callee-saved-reg spills are inserted at front afterwards
  MachineBasicBlock &EntryMBB = MF.front();

  DebugLoc DL;

  // Do not emit anything for naked functions
  if (MF.getFunction().hasFnAttribute("naked")) {
    return;
  }

  if (hasFP(MF)) {
    // if framepointer enabled, set it to point to the stack pointer.
    // Set frame pointer: FP = SP
    AddDefaultPred(BuildMI(EntryMBB, EntryMBB.begin(), DL,
          TII->get(Patmos::MOV), Patmos::RFP)).addReg(Patmos::RSP);
    // Mark RFP as used
    SavedRegs.set(Patmos::RFP);
  }

  // mark all predicate registers as used, for single path support
  // S0 is saved/restored as whole anyway
  if (PatmosSinglePathInfo::isEnabled(MF)) {
    SavedRegs.set(Patmos::S0);
    SavedRegs.set(Patmos::R26);
  }

  if (TRI->requiresRegisterScavenging(MF)) {
    const TargetRegisterClass &RC = *TRI->getRegClass(Patmos::RRegsRegClassID);
    int fi = MFI.CreateStackObject(TRI->getSpillSize(RC), TRI->getSpillAlign(RC), false);
    RS->addScavengingFrameIndex(fi);
    PMFI.setRegScavengingFI(fi);
  }
}

/// Gets the general-purpose register that should be used to spill/restore
/// the given special-purpose register.
static Register get_special_spill_restore_reg(Register Reg)
{
	assert(Patmos::SRegsRegClass.contains(Reg));

	switch (Reg) {
	case Patmos::S0:
		return Patmos::R9;
	case Patmos::S1:
		return Patmos::R10;
	case Patmos::SL:
		return Patmos::R11;
	case Patmos::SH:
		return Patmos::R12;
	case Patmos::S4:
		return Patmos::R13;
	case Patmos::SS:
		return Patmos::R14;
	case Patmos::ST:
		return Patmos::R15;
	case Patmos::SRB:
		return Patmos::R16;
	case Patmos::SRO:
		return Patmos::R17;
	case Patmos::SXB:
		return Patmos::R18;
	case Patmos::SXO:
		return Patmos::R19;
	case Patmos::S11:
		return Patmos::R20;
	case Patmos::S12:
		return Patmos::R19; // We have run out of scratch registers so we reuse
	case Patmos::S13:
		return Patmos::R18;
	case Patmos::S14:
		return Patmos::R17;
	case Patmos::S15:
		return Patmos::R16;
	default:
		assert(false && "Unknown special-purpose register");
		return 0;
	}
}

bool
PatmosFrameLowering::spillCalleeSavedRegisters(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
    ArrayRef<CalleeSavedInfo> CSI, Register TRI) const {
  if (CSI.empty())
    return false;

  DebugLoc DL;
  if (MI != MBB.end()) DL = MI->getDebugLoc();

   MachineFunction &MF = *MBB.getParent();
   const TargetInstrInfo &TII = *STC.getInstrInfo();
   const TargetRegisterInfo *TRIptr = STC.getRegisterInfo();

  // Start by spilling general-purpose registers
  std::vector<Register> rreg_spilled;
  std::map<Register, unsigned> sreg_unspilled; // Special-purpose register in need of spilling and their frame_idx
   for (unsigned i = 0; i < CSI.size(); i++) {
	    auto Reg = CSI[i].getReg();
	    auto frame_idx = CSI[i].getFrameIdx();
	    // Add the callee-saved register as live-in. It's killed at the spill.
	    MBB.addLiveIn(Reg);

        // Use TRI to lookup register classes by ID (modern LLVM)
        if (TRIptr->getRegClass(Patmos::RRegsRegClassID)->contains(Reg)) {

    		// spill: obtain a physical register class for this register
        const TargetRegisterClass *RC = TRIptr->getMinimalPhysRegClass(Reg);
        TII.storeRegToStackSlot(MBB, MI, Reg, true,
                frame_idx, RC, Register());
    		std::prev(MI)->setFlag(MachineInstr::FrameSetup);

    		rreg_spilled.push_back(Reg);
        } else if (TRIptr->getRegClass(Patmos::PRegsRegClassID)->contains(Reg)) {
    		// Fixed lambda: must return the comparison result
    		assert(std::any_of(CSI.begin(), CSI.end(), [&](const CalleeSavedInfo &info){
    			return info.getReg() == Patmos::S0;
    		}) && "Must spill S0 instead of individual predicate registers");
    	} else {
        assert(TRIptr->getRegClass(Patmos::SRegsRegClassID)->contains(Reg));

    		// We don't spill special-purpose register now, as we want to know which
    		// general-purpose registers we're spilling, so we can reuse them for
    		// spilling the specials.
    		// This is crucial for single-path code, because a disabled function can't change any registers.
    		// Therefore we know we can use the spilled general-purpose registers since they will later be restored.
    		sreg_unspilled[Reg] = frame_idx;
    	}
   }

  // Spill special-purpose registers
  for(auto entry: sreg_unspilled) {
	  auto reg = entry.first;
	  auto frame_idx = entry.second;

	  // copy sreg to rreg
	  // We make each sreg use a different rreg to allow more flexible scheduling
	  Register tmpReg;
	  if(rreg_spilled.empty()) {
		  auto MF = MBB.getParent();
		  assert((!PatmosSinglePathInfo::isEnabled(*MF) || !PatmosSinglePathInfo::isRootLike(*MF)) &&
				  "Couldn't find general-purpose register to use for spilling special-purpose register");
		  tmpReg = get_special_spill_restore_reg(reg);
	  } else {
		  tmpReg = rreg_spilled[reg.id()%rreg_spilled.size()];
	  }

	  TII.copyPhysReg(MBB, MI, DL, tmpReg, reg, true);
	  std::prev(MI)->setFlag(MachineInstr::FrameSetup);
	  // Spill the value
      TII.storeRegToStackSlot(MBB, MI, tmpReg, true,
                frame_idx, TRIptr->getRegClass(Patmos::RRegsRegClassID), Register());
		std::prev(MI)->setFlag(MachineInstr::FrameSetup);
  }

  return true;
}

bool
PatmosFrameLowering::restoreCalleeSavedRegisters(MachineBasicBlock &MBB,
                                        MachineBasicBlock::iterator MI,
                                        MutableArrayRef<CalleeSavedInfo> CSI,
                                        const TargetRegisterInfo *TRI) const {
  if (CSI.empty())
    return false;

  DebugLoc DL;
  if (MI != MBB.end()) DL = MI->getDebugLoc();

  MachineFunction &MF = *MBB.getParent();
  const TargetInstrInfo &TII = *STC.getInstrInfo();
  PatmosMachineFunctionInfo &PMFI = *MF.getInfo<PatmosMachineFunctionInfo>();

  // if frame-pointer enabled, first restore the stack pointer.
  if (hasFP(MF)) {
    // Restore stack pointer: SP = FP
    AddDefaultPred(BuildMI(MBB, MI, DL, TII.get(Patmos::MOV), Patmos::RSP))
      .addReg(Patmos::RFP);
  }

  // We need to restore the special-purpose registers first, so sort the list of register
  // into general and special (with their frame-indices)
  std::vector<std::pair<Register, unsigned>> rregs;
  std::map<Register, unsigned> sregs;
  for (unsigned i = 0; i < CSI.size(); i++) {
    auto Reg = CSI[i].getReg();
    auto frame_idx = CSI[i].getFrameIdx();
    if (TRI->getRegClass(Patmos::RRegsRegClassID)->contains(Reg)) {
      rregs.push_back(std::make_pair(Reg, frame_idx));
    } else if (TRI->getRegClass(Patmos::PRegsRegClassID)->contains(Reg)) {
      assert(std::any_of(CSI.begin(), CSI.end(), [&](const CalleeSavedInfo &info){
        return info.getReg() == Patmos::S0;
      }) && "Must restore S0 instead of individual predicate registers");
    } else {
        assert(TRI->getRegClass(Patmos::SRegsRegClassID)->contains(Reg));
      sregs[Reg] = frame_idx;
    }
  }

  // Restore special-purpose registers
  for(auto entry: sregs) {
	  auto reg = entry.first;
	  auto frame_idx = entry.second;

	  // We make each sreg use a different rreg for reloading to allow more flexible scheduling
	  Register tmpReg;
	  if(rregs.empty()) {
		  auto MF = MBB.getParent();
		  assert((!PatmosSinglePathInfo::isEnabled(*MF) || !PatmosSinglePathInfo::isRootLike(*MF)) &&
				  "Couldn't find general-purpose register to use for restoring special-purpose register");
		  tmpReg = get_special_spill_restore_reg(reg);
	  } else {
		  tmpReg = rregs[reg.id()%rregs.size()].first;
	  }

	  // load
      TII.loadRegFromStackSlot(MBB, MI, tmpReg, frame_idx, TRI->getRegClass(Patmos::RRegsRegClassID), Register());
	  std::prev(MI)->setFlag(MachineInstr::FrameSetup);

	  // Move value to special register
	  TII.copyPhysReg(MBB, MI, DL, reg, tmpReg, true);
	  std::prev(MI)->setFlag(MachineInstr::FrameSetup);
  }

  // Restore general-purpose registers
  for(auto entry: rregs) {
	  auto reg = entry.first;
	  auto frame_idx = entry.second;

	  // load
      TII.loadRegFromStackSlot(MBB, MI, reg, frame_idx, TRI->getRegClass(Patmos::RRegsRegClassID), Register());
	  std::prev(MI)->setFlag(MachineInstr::FrameSetup);
  }

  return true;
}

MachineBasicBlock::iterator
PatmosFrameLowering::eliminateCallFramePseudoInstr(MachineFunction &MF,
                                                   MachineBasicBlock &MBB,
                                                   MachineBasicBlock::iterator I) const {
  const TargetInstrInfo &TII = *STC.getInstrInfo();

  // We need to adjust the stack pointer here (and not in the prologue) to
  // handle alloca instructions that modify the stack pointer before ADJ*
  // instructions. We only need to do that if we need a frame pointer, otherwise
  // we reserve size for the call stack frame in FrameLowering in the prologue.
  if (hasFP(MF)) {
    MachineInstr &MI = *I;
    DebugLoc dl = MI.getDebugLoc();
    int Size = MI.getOperand(0).getImm();
    unsigned Opcode;
    if (MI.getOpcode() == Patmos::ADJCALLSTACKDOWN) {
      Opcode = (Size <= 0xFFF) ? Patmos::SUBi : Patmos::SUBl;
    }
    else if (MI.getOpcode() == Patmos::ADJCALLSTACKUP) {
      Opcode = (Size <= 0xFFF) ? Patmos::ADDi : Patmos::ADDl;
    }
    else {
        llvm_unreachable("Unsupported pseudo instruction.");
    }
    if (Size)
      AddDefaultPred(BuildMI(MBB, I, dl, TII.get(Opcode), Patmos::RSP))
                                                  .addReg(Patmos::RSP)
                                                  .addImm(Size);
  }
  // erase the pseudo instruction
  return MBB.erase(I);
}
