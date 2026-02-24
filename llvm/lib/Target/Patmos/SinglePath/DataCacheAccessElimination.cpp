//===-- DataCacheAccessElimination.cpp - Remove unused function declarations ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass makes the single-pat code utilitize Patmos' dual issue pipeline.
// TODO: more description
//
//===----------------------------------------------------------------------===//

#include "DataCacheAccessElimination.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"

using namespace llvm;

STATISTIC(LoadsConverted,     "Number of data-cache loads converted to direct main memory loads");
STATISTIC(SavesConverted,     "Number of data-cache saves converted to direct main memory saves");

char DataCacheAccessElimination::ID = 0;

/// createDataCacheAccessEliminationPass - Returns a new DataCacheAccessElimination
/// \see DataCacheAccessElimination
FunctionPass *llvm::createDataCacheAccessEliminationPass(const PatmosTargetMachine &tm) {
  return new DataCacheAccessElimination(tm);
}

bool DataCacheAccessElimination::runOnMachineFunction(MachineFunction &MF) {
  // only convert function if marked
  if ( PatmosSinglePathInfo::isEnabled(MF) ) {
    eliminateDCAccesses(MF);
  }

  return true;
}

void DataCacheAccessElimination::eliminateDCAccesses(MachineFunction &MF)
{
  for(auto BB_iter = MF.begin(), BB_iter_end = MF.end(); BB_iter != BB_iter_end; ++BB_iter){
    for(auto instr_iter = BB_iter->begin(), instr_iter_end = BB_iter->end(); instr_iter != instr_iter_end; ++instr_iter){
      unsigned convert_to;
      switch( instr_iter->getOpcode() ){
        case Patmos::LWC:
          LoadsConverted++;
          convert_to = Patmos::LWM;
          break;
        case Patmos::LHC:
          LoadsConverted++;
          convert_to = Patmos::LHM;
          break;
        case Patmos::LBC:
          LoadsConverted++;
          convert_to = Patmos::LBM;
          break;
        case Patmos::LHUC:
          LoadsConverted++;
          convert_to = Patmos::LHUM;
          break;
        case Patmos::LBUC:
          LoadsConverted++;
          convert_to = Patmos::LBUM;
          break;
        case Patmos::SWC:
          SavesConverted++;
          convert_to = Patmos::SWM;
          break;
        case Patmos::SHC:
          SavesConverted++;
          convert_to = Patmos::SHM;
          break;
        case Patmos::SBC:
          SavesConverted++;
          convert_to = Patmos::SBM;
          break;
        default:
          // Not a data cache accesses, ignore
          continue;
      }

      // Create new instruction accessing main memory instead
      auto *new_instr = MF.CreateMachineInstr( TII->get(convert_to), instr_iter->getDebugLoc());
      MachineInstrBuilder new_instr_builder(MF, new_instr);

      // Give it the same operands
      for(auto op: instr_iter->operands()) {
        new_instr_builder.add(op);
      }

      // Replace old instruction by new one in BB
      BB_iter->insertAfter(instr_iter, new_instr);
      instr_iter = BB_iter->erase(&*instr_iter);
    }
  }
}


