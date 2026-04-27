//===-- PatmosAsmBackend.cpp - Patmos Asm Backend  ----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the PatmosAsmBackend class.
//
//===----------------------------------------------------------------------===//
//

#include "MCTargetDesc/PatmosAsmBackend.h"
#include "MCTargetDesc/PatmosFixupKinds.h"
#include "MCTargetDesc/PatmosMCTargetDesc.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDirectives.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/EndianStream.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace Patmos;

// Prepare value for the target space for it
static unsigned adjustFixupValue(const MCFixup &Fixup, uint64_t Value) {

  unsigned Kind = Fixup.getKind();

  // Add/subtract and shift
  switch (Kind) {
  // TODO check: do we need to shift the load/store offsets here or is this done
  // earlier in the compiler?
  case FK_Patmos_HO_7:
    Value >>= 1;
    break;
  case FK_Patmos_WO_7:
  case FK_Patmos_abs_CFLi:
  case FK_Patmos_PCrel:
    Value >>= 2;
    break;
  }

  return Value;
}

std::unique_ptr<MCObjectTargetWriter>
PatmosAsmBackend::createObjectTargetWriter() const {
  return createPatmosELFObjectWriter(TheTriple);
}

// ApplyFixup - Apply the Value for given Fixup into the provided data fragment,
// at the offset specified by the fixup and following the fixup kind as appropriate.
void PatmosAsmBackend::applyFixup(const MCFragment &Fragment, const MCFixup &Fixup,
                                  const MCValue &Target, uint8_t *Data,
                                  uint64_t Value, bool IsResolved) {
  MCFixupKind Kind = Fixup.getKind();

  // Modern MC requires targets to explicitly emit relocations for unresolved
  // fixups. Without this, symbol references silently encode as zero.
  if (!IsResolved)
    Asm->getWriter().recordRelocation(Fragment, Fixup, Target, Value);

  if (mc::isRelocation(Kind))
    return;

  const MCFixupKindInfo Info = getFixupKindInfo(Kind);
  Value = adjustFixupValue(Fixup, Value);

  if (!Value)
    return; // Doesn't change encoding.

  // Shift encoded bits into their final instruction position.
  Value <<= Info.TargetOffset;

  const unsigned NumBytes = alignTo(Info.TargetSize + Info.TargetOffset, 8) / 8;
  const unsigned KindByteOffset =
      (Kind == FK_Patmos_abs_ALUl) ? 4u : 0u;

  if (Fixup.getOffset() + KindByteOffset + NumBytes > Fragment.getSize()) {
    getContext().reportError(Fixup.getLoc(), "invalid fixup offset");
    return;
  }

  Data += KindByteOffset;

  // Data points at fragment contents + Fixup.getOffset(). Patmos encodes bytes
  // in big-endian order.
  for (unsigned i = 0; i != NumBytes; ++i) {
    const unsigned Idx = NumBytes - 1 - i;
    Data[Idx] |= static_cast<uint8_t>((Value >> (i * 8)) & 0xff);
  }
}

MCFixupKindInfo PatmosAsmBackend::getFixupKindInfo(MCFixupKind Kind) const {
  static const MCFixupKindInfo Infos[Patmos::NumTargetFixupKinds] = {
    // This table *must* be in same the order of FK_* kinds in
    // PatmosFixupKinds.h.
    //
    // name                    offset  bits  flags
    { "FK_Patmos_BO_7" ,       25,      7,   0 }, // 0 bit shifted, unsigned (byte aligned)
    { "FK_Patmos_HO_7" ,       25,      7,   0 }, // 1 bit shifted, unsigned (half-word aligned)
    { "FK_Patmos_WO_7" ,       25,      7,   0 }, // 2 bit shifted, unsigned (word aligned)
    { "FK_Patmos_abs_ALUi",    20,     12,   0 }, // ALU immediate, unsigned
    { "FK_Patmos_abs_CFLi",    10,     22,   0 }, // 2 bit shifted, unsigned, for call
    { "FK_Patmos_abs_ALUl",     0,     32,   0 }, // ALUl immediate (reloc anchored at insn start)
    { "FK_Patmos_stc",         14,     18,   0 }, // 2 bit shifted, unsigned, for stack control
    // Modern LLVM tracks PC-relativity on the MCFixup itself (Fixup.isPCRel()),
    // so the Flags field in MCFixupKindInfo does not define FKF_IsPCRel anymore.
    // Use 0 here for the flags field... I hope...
    { "FK_Patmos_PCrel",       10,     22,   0 }, // 2 bit shifted, signed, PC relative
  };
  static_assert(std::size(Infos) == Patmos::NumTargetFixupKinds,
                "Fixup kind info table is out of sync with Patmos::Fixups");

  if (Kind < FirstTargetFixupKind)
    return MCAsmBackend::getFixupKindInfo(Kind);

  assert(unsigned(Kind - FirstTargetFixupKind) < Patmos::NumTargetFixupKinds &&
         "Invalid kind!");

  return Infos[Kind - FirstTargetFixupKind];
}

/// WriteNopData - Write an (optimal) nop sequence of Count bytes
/// to the given output. If the target cannot generate such a sequence,
/// it should return an error.
///
/// \return - True on success.
bool PatmosAsmBackend::writeNopData(raw_ostream &OS, uint64_t Count, const MCSubtargetInfo *STI) const {
  // Count is in terms of of ValueSize, which is always 1 Byte for ELF.
  // This method is used to create a NOP slide for code segment alignment  
  // OW handles byteorder stuff.

  if ((Count % 4) != 0)
    return false;

  // We should somehow initialize the NOP instruction code from TableGen, but
  // I do not see how (without creating a new CodeEmitter and everything from
  // Target)

  for (uint64_t i = 0; i < Count; i += 4)
    // "(p0) sub r0 = r0, 0"
    support::endian::write<uint32_t>(OS, 0x00400000, llvm::endianness::big);

  return true;
}

MCAsmBackend *llvm::createPatmosAsmBackend(const Target &T,
                                           const MCSubtargetInfo &STI,
                                           const MCRegisterInfo &MRI,
                                           const MCTargetOptions &Options) {
  return new PatmosAsmBackend(T, MRI, STI.getTargetTriple(), STI.getCPU());
}
