//===-- PatmosAsmBackend.h - Patmos Asm Backend  ------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines the PatmosAsmBackend class.
//
//===----------------------------------------------------------------------===//
//

#ifndef LLVM_LIB_TARGET_PATMOS_MCTARGETDESC_PATMOSASMBACKEND_H
#define LLVM_LIB_TARGET_PATMOS_MCTARGETDESC_PATMOSASMBACKEND_H

#include "MCTargetDesc/PatmosFixupKinds.h"
#include "llvm/TargetParser/Triple.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/Support/Endian.h"

namespace llvm {

class MCAssembler;
struct MCFixupKindInfo;
class MCRegisterInfo;
class Target;

class PatmosAsmBackend : public MCAsmBackend {
  Triple TheTriple;

public:
  PatmosAsmBackend(const Target &T, const MCRegisterInfo &MRI, const Triple &TT,
                   StringRef CPU)
      : MCAsmBackend(llvm::endianness::big), TheTriple(TT) {}

  std::unique_ptr<MCObjectTargetWriter>
  createObjectTargetWriter() const override;

  void applyFixup(const MCFragment &, const MCFixup &, const MCValue &Target,
                  uint8_t *Data, uint64_t Value, bool IsResolved) override;

  MCFixupKindInfo getFixupKindInfo(MCFixupKind Kind) const override;

  /// @name Target Relaxation Interfaces
  /// @{

  /// MayNeedRelaxation - Check whether the given instruction may need
  /// relaxation.
  ///
  /// \param Inst - The instruction to test.
  bool mayNeedRelaxation(unsigned Opcode, ArrayRef<MCOperand> Operands,
                       const MCSubtargetInfo &STI) const override {
    // TODO return true for small immediates (?)
    return false;
  }



  /// fixupNeedsRelaxation - Target specific predicate for whether a given
  /// fixup requires the associated instruction to be relaxed.
  bool fixupNeedsRelaxationAdvanced(const MCFragment &, const MCFixup &,
                                    const MCValue &, uint64_t,
                                    bool) const override {
    // FIXME.
    llvm_unreachable("RelaxInstruction() unimplemented");
    return false;
  }

  bool writeNopData(raw_ostream &OS, uint64_t Count, const MCSubtargetInfo *STI) const override;

}; // class PatmosAsmBackend

} // namespace

#endif
