//===-- PatmosTargetStreamer.cpp - Patmos Target Streamer Methods ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file provides Patmos specific target streamer methods.
//
//===----------------------------------------------------------------------===//

#include "PatmosTargetStreamer.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCSymbolELF.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/MC/MCSubtargetInfo.h"

using namespace llvm;

// pin vtable to this file
void PatmosTargetStreamer::anchor() {}

PatmosTargetStreamer::PatmosTargetStreamer(MCStreamer &S) : MCTargetStreamer(S) {}

PatmosTargetAsmStreamer::PatmosTargetAsmStreamer(MCStreamer &S,
                                                 llvm::formatted_raw_ostream &OS)
    : PatmosTargetStreamer(S), OS(OS) {}

void PatmosTargetAsmStreamer::emitFStart(const MCSymbol *Start,
                               const MCExpr* Size, Align Alignment)
{
  // Print the start symbol and the size expression. Use MCExpr::print to
  // handle all expression kinds correctly (pass nullptr for MCAsmInfo as
  // this is used by dump() and some asm printing paths).
  OS << "\t.fstart\t" << *Start << ", ";
  if (Size)
    getContext().getAsmInfo().printExpr(OS, *Size);
  else
    OS << "<null>";
  OS << ", " << Alignment.value() << "\n";
}

PatmosTargetELFStreamer::PatmosTargetELFStreamer(MCStreamer &S)
    : PatmosTargetStreamer(S) {}

void PatmosTargetELFStreamer::emitFStart(const MCSymbol *Start,
      const MCExpr* Size, Align Alignment)
{
  // Pass the subtarget info as nullptr (no STI available here)
  if (const MCSubtargetInfo *STI = getStreamer().getContext().getSubtargetInfo()) {
    getStreamer().emitCodeAlignment(Align(Alignment.value()), *STI);
  } else {
    // Safe fallback if MCContext doesn't map a subtarget context during this phase,
    // this used to be a nullptr before before upgrading from LLVM 22 to LLVM23
    report_fatal_error("MCContext is missing SubtargetInfo inside PatmosTargetStreamer.");
  }
  getStreamer().emitValue(Size, 4);

}
