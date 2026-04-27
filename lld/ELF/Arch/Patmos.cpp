//===- Patmos.cpp ----------------------------------------------------------===//
//
// Addaption of File lld/ELF/Arch/RISCV.cpp of the LLVM Project, 
// under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "InputFiles.h"
#include "Symbols.h"
#include "SyntheticSections.h"
#include "Target.h"
#include "llvm/Support/MathExtras.h"

using namespace llvm;
using namespace llvm::object;
using namespace llvm::support::endian;
using namespace llvm::ELF;
using namespace lld;
using namespace lld::elf;

namespace {

class Patmos final : public TargetInfo {
public:
  Patmos(Ctx &ctx);
  RelExpr getRelExpr(RelType type, const Symbol &s,
                     const uint8_t *loc) const override;
  int64_t getImplicitAddend(const uint8_t *buf, RelType type) const override;
  void relocate(uint8_t *loc, const Relocation &rel,
                uint64_t val) const override;
};

}

Patmos::Patmos(Ctx &ctx) : TargetInfo(ctx) {
  defaultImageBase = 0x00020000;
  
  defaultMaxPageSize = 0x1000;
  defaultCommonPageSize = 0x1000;
}

// getRelExpr - decide what value is needed depending on the relocation type (absolute or relative)
//// @param loc   Location Pointer to data/instruction the patching is needed
//// @param s     Symbol
//// @param type  What Relocation Type needed to be applied
RelExpr Patmos::getRelExpr(const RelType type, const Symbol &s,
                          const uint8_t *loc) const {

  // get which Relocation Value is returned
  switch (type) {
  case R_PATMOS_NONE:
    return R_NONE;
  case R_PATMOS_CFLI_ABS:
    return R_ABS; // Absolute Address 
  case R_PATMOS_ALUI_ABS:
    return R_ABS;  
  case R_PATMOS_ALUL_ABS:
    return R_ABS; 
  case R_PATMOS_MEMB_ABS:
    return R_ABS; 
  case R_PATMOS_MEMH_ABS:
    return R_ABS; 
  case R_PATMOS_MEMW_ABS:
    return R_ABS; 
  case R_PATMOS_ABS_32:
    return R_ABS;  
  case R_PATMOS_CFLI_PCREL:
    return R_PC; // Relative Address
  default:
    // Damn LLVM syntax updates! To heck!
    Err(ctx) << getErrorLoc(ctx, loc) << "unknown relocation (" << type.v
             << ") against symbol " << &s;
    return R_NONE;
  }
}

// Extract bits V[Begin:End], where range is inclusive, and Begin must be < 63.
static uint32_t extractBits(uint64_t v, uint32_t begin, uint32_t end) {
  return (v & ((1ULL << (begin + 1)) - 1)) >> end;
}

int64_t Patmos::getImplicitAddend(const uint8_t *buf, RelType type) const {
  switch (type) {
  case R_PATMOS_NONE:
    return 0;
  case R_PATMOS_CFLI_ABS:
    return static_cast<int64_t>(read32be(buf) & 0x3FFFFF) << 2;
  case R_PATMOS_CFLI_PCREL: {
    int64_t imm = SignExtend64<22>(read32be(buf) & 0x3FFFFF);
    return imm << 2;
  }
  case R_PATMOS_ALUI_ABS:
    return read32be(buf) & 0xFFF;
  case R_PATMOS_ALUL_ABS:
    return static_cast<int64_t>(read64be(buf) & 0xFFFFFFFFULL);
  case R_PATMOS_MEMB_ABS:
    return read32be(buf) & 0x7F;
  case R_PATMOS_MEMH_ABS:
    return static_cast<int64_t>(read32be(buf) & 0x7F) << 1;
  case R_PATMOS_MEMW_ABS:
    return static_cast<int64_t>(read32be(buf) & 0x7F) << 2;
  case R_PATMOS_ABS_32:
    return read32be(buf);
  default:
    return TargetInfo::getImplicitAddend(buf, type);
  }
}


// relocate - patch data/instruction depending on the relocation type
//// @param loc   Location Pointer to data/instruction where patching is needed
//// @param rel   Relocation Type to be applied
//// @param val   Value to be patched to the data/instruction 
void Patmos::relocate(uint8_t *loc, const Relocation &rel, uint64_t val) const {
  // Relocate Types defined in /llvm/include/llvm/BinaryFormat/ELF.h
  
  switch (rel.type) {
  case R_PATMOS_CFLI_ABS: {
    checkUInt(ctx, loc, static_cast<int64_t>(val) >> 2, 22, rel);

    const uint32_t mask = 0x3FFFFF;
    uint32_t insn = read32be(loc) & ~(mask);
    uint32_t imm =  extractBits(val, 23, 2);
    insn |= imm;
    write32be(loc,insn);
    return;
  }
  case R_PATMOS_ALUI_ABS: {
    checkUInt(ctx, loc, static_cast<int64_t>(val), 12, rel);

    const uint32_t mask = 0xFFF;
    uint32_t insn = read32be(loc) & ~(mask);
    uint32_t imm =  extractBits(val, 11, 0);
    insn |= imm;
    write32be(loc,insn);
    return;
  }
  case R_PATMOS_ALUL_ABS: {
    checkUInt(ctx, loc, static_cast<int64_t>(val), 32, rel);

    const uint64_t mask = 0xFFFFFFFF;
    uint64_t insn = read64be(loc) & ~(mask);
    uint32_t imm =  extractBits(val, 31, 0);
    insn |= imm;
    write64be(loc,insn);
    return;
  }
  case R_PATMOS_MEMB_ABS:{
    checkInt(ctx, loc, static_cast<int64_t>(val), 7, rel);

    const uint64_t mask = 0x7F;
    uint32_t insn = read32be(loc) & ~(mask);
    uint32_t imm =  extractBits(val, 6, 0);
    insn |= imm;
    write32be(loc,insn);
    return;
  }
  case R_PATMOS_MEMH_ABS:{
    // Relocate LDT or STT format (7 bit immediate), absolute, signed, in half-words
    checkInt(ctx, loc, static_cast<int64_t>(val) >> 1, 7, rel);

    const uint64_t mask = 0x7F;
    uint32_t insn = read32be(loc) & ~(mask);
    uint32_t imm =  extractBits(val, 7, 1);
    insn |= imm;
    write32be(loc,insn);
    return;
  }
  case R_PATMOS_MEMW_ABS:{
    // Relocate LDT or STT format (7 bit immediate), absolute, signed, in words
    checkInt(ctx, loc, static_cast<int64_t>(val) >> 2, 7, rel);

    const uint64_t mask = 0x7F;
    uint32_t insn = read32be(loc) & ~(mask);
    uint32_t imm =  extractBits(val, 8, 2);
    insn |= imm;
    write32be(loc,insn);
    return;
  }
  case R_PATMOS_ABS_32: {
    // Relocate 32 bit word, absolute (unsigned), in bytes
    checkUInt(ctx, loc, static_cast<int64_t>(val), 32, rel);

    uint64_t out =  read32be(loc);
    uint32_t temp =  extractBits(val, 31, 0);
    out += temp;

    checkIntUInt(ctx, loc, out, 32, rel);

    write32be(loc,out);
    
    return;
  }
  case R_PATMOS_CFLI_PCREL: {
    // Relocate CFLi format (22 bit immediate), PC relative (signed), in words
    checkInt(ctx, loc, static_cast<int64_t>(val) >> 2, 22, rel);

    const uint32_t mask = 0x3FFFFF;
    uint32_t insn = read32be(loc) & ~(mask);
    uint32_t imm =  extractBits(val, 23, 2);
    insn |= imm;
    write32be(loc,insn);
    return;
  }
  default:
    llvm_unreachable("unknown relocation");
  }
}

void elf::setPatmosTargetInfo(Ctx &ctx) { ctx.target.reset(new Patmos(ctx)); }
