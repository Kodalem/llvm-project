//===-- PatmosExport.cpp - Target-specific PML exporter for Patmos --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains a Patmos-customized PML export driver and -pass.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "patmos-export"

#include "Patmos.h"
#include "PatmosInstrInfo.h"
#include "PatmosMachineFunctionInfo.h"
#include "PatmosStackCacheAnalysis.h"
#include "PatmosTargetMachine.h"
#include "PMLExport.h"
#include "InstPrinter/PatmosInstPrinter.h"
#include "TargetInfo/PatmosTargetInfo.h"
#include "SinglePath/PatmosSinglePathInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/CodeGen/Analysis.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/MachineJumpTableInfo.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"

#include <map>
#include <sstream>
#include <iostream>

using namespace llvm;

namespace llvm {

  class PatmosPMLInstrInfo : public PMLInstrInfo {

    PatmosTargetMachine &TM;

  public:
    PatmosPMLInstrInfo(PatmosTargetMachine &tm) : PMLInstrInfo(), TM(tm) {}

    unsigned getBranchDelaySlots(const MachineInstr *Instr) override {
      switch (Instr->getOpcode()) {
      case Patmos::BR:
      case Patmos::BRu: 
      case Patmos::BRR:
      case Patmos::BRRu: 
      case Patmos::BRT:
      case Patmos::BRTu:
		return 2;
      case Patmos::BRCF:
      case Patmos::BRCFu:
      case Patmos::BRCFR:
      case Patmos::BRCFRu:
      case Patmos::BRCFT:
      case Patmos::BRCFTu:
      case Patmos::CALL:
      case Patmos::CALLR:
      case Patmos::RET:
      case Patmos::XRET:
		return 3;
      default:
		return 0;
      }
    }

    const std::vector<MachineBasicBlock*> getBranchTargets(
                                    MachineFunction &MF,
                                    const MachineInstr *Instr) override
    {
      std::vector<MachineBasicBlock*> targets;

      switch (Instr->getOpcode()) {
      case Patmos::BR:
      case Patmos::BRu:
      case Patmos::BRCF:
      case Patmos::BRCFu:
      case Patmos::BRND:
      case Patmos::BRNDu:
      case Patmos::BRCFND:
      case Patmos::BRCFNDu: {
        // handle normal branches, return single branch target
        const MachineOperand &MO(Instr->getOperand(2));

        if (MO.isMBB()) {
          targets.push_back( MO.getMBB() );
        }

        break;
        }
      case Patmos::BRT:
      case Patmos::BRTu:
      case Patmos::BRCFT:
      case Patmos::BRCFTu:
      case Patmos::BRTND:
      case Patmos::BRTNDu:
      case Patmos::BRCFTND:
      case Patmos::BRCFTNDu: {
        // read jump table (patmos specific: operand[3] of BR(CF)?Tu?)
        assert(Instr->getNumOperands() == 4);

        unsigned index = Instr->getOperand(3).getIndex();
        MachineJumpTableInfo *MJTI = MF.getJumpTableInfo();

        typedef const std::vector<MachineBasicBlock*> JTEntries;
        JTEntries &JTBBs(MJTI->getJumpTables()[index].MBBs);

        return JTBBs;
        }
      }

      return targets;
    }

    int getSize(const MachineInstr *Instr) override
    {
      return TM.getInstrInfo()->getInstrSize(Instr);
    }
  };

  class PatmosBitcodeExport : public PMLBitcodeExport {
  public:
    PatmosBitcodeExport(PatmosTargetMachine &tm, ModulePass &mp)
      : PMLBitcodeExport(tm, mp) {}

    bool doExportInstruction(const Instruction* Instr) override {
      return true;
    }
  };

  // we need information about the calling conventions!
  #include "PatmosGenCallingConv.inc"

  class PatmosMachineExport : public PMLMachineExport {

  public:
    PatmosMachineExport(PatmosTargetMachine &tm, ModulePass &mp, const TargetInstrInfo *TII,
                        PMLInstrInfo *PII)
      : PMLMachineExport(tm, mp, TII, PII) {
        // silence compiler warning
        (void)RetCC_Patmos;
      }


    bool doExportInstruction(const MachineInstr *Ins) override {
      return true;
    }

     void exportInstruction(MachineFunction &MF,
                                   yaml::MachineInstruction *I,
                                   const MachineInstr *Instr,
                                   bool BundledWithPred) override;

    /// exportArgumentRegisterMapping
    /// see below for implementation
    void exportArgumentRegisterMapping(
                                  yaml::PMLMachineFunction *PMF,
                                  const MachineFunction &MF) override;

    void exportSubfunctions(MachineFunction &MF,
                                        yaml::PMLMachineFunction *PMF) override;

    void exportLoopInfo(MachineFunction &MF,
                                yaml::PMLDoc<yaml::PMLMachineFunction,yaml::UnsignedValue> &YDoc,
                                MachineLoop *Loop) override;
  };


  class PatmosModuleExportPass : public PMLModuleExportPass {
    static char ID;

  public:
    PatmosModuleExportPass(PatmosTargetMachine &tm, StringRef filename,
                           ArrayRef<std::string> roots, bool SerializeAll)
      : PMLModuleExportPass(ID, tm, filename, roots, SerializeAll)
    {
      initializePatmosCallGraphBuilderPass(*PassRegistry::getPassRegistry());

      setPMLInstrInfo(new PatmosPMLInstrInfo(tm));
    }

    PatmosPMLInstrInfo *getPatmosInstrInfo() {
      return static_cast<PatmosPMLInstrInfo*>(getPMLInstrInfo());
    }

    StringRef getPassName() const override {
      return "Patmos YAML/PML Module Export";
    }

    void getAnalysisUsage(AnalysisUsage &AU) const override {
      AU.setPreservesAll();
      AU.addRequired<PatmosStackCacheAnalysisInfo>();
      PMLModuleExportPass::getAnalysisUsage(AU);
    }

    bool runOnModule(Module &M) override {
      return PMLModuleExportPass::runOnModule(M);
    }
  };

  char PatmosModuleExportPass::ID = 0;


  /// createPatmosExportPass - Returns a new PatmosExportPass
  /// \see PatmosExportPass
  ModulePass *createPatmosModuleExportPass(PatmosTargetMachine &TM,
                                           std::string& Filename,
                                           std::string& BitcodeFilename,
                                           ArrayRef<std::string> Roots,
					   bool SerializeAll)
  {
    PatmosModuleExportPass *PEP =
                      new PatmosModuleExportPass(TM, Filename, Roots, SerializeAll);

    // Add our own export passes
    PEP->addExporter( new PatmosMachineExport(TM, *PEP, TM.getSubtargetImpl()->getInstrInfo(), PEP->getPatmosInstrInfo()));
    PEP->addExporter( new PatmosBitcodeExport(TM, *PEP) );
    PEP->addExporter( new PMLRelationGraphExport(TM, *PEP) );
    if (! BitcodeFilename.empty())
      PEP->writeBitcode(BitcodeFilename);
    return PEP;
  }



///////////////////////////////////////////////////////////////////////////////


  void PatmosMachineExport::
  exportArgumentRegisterMapping(yaml::PMLMachineFunction *PMF,
                                const MachineFunction &MF)
  {

    const PatmosTargetLowering *TLI = TM.getSubtargetImpl()->getTargetLowering();
      auto &F = MF.getFunction();
      LLVMContext &Ctx = F.getParent()->getContext();

      if (F.isVarArg()) {
        // we are passing arguments on stack, can't handle this for now
        // TODO
        return;
      }

      // following code segment was copied in large parts from
      // SelectionDAGISel::LowerArguments(), which can be found in
      // lib/CodeGen/SelectionDAG/SelectionDAGBuilder.cpp:6628
      /////////////////

      SmallVector<ISD::InputArg, 16> Ins;
      const DataLayout &TD = MF.getDataLayout();
      ISD::ArgFlagsTy Flags;

      // For Patmos, PatmosISelLowering does not overload CanLowerReturn(),
      // which returns true by default and is queried by FunctionInfo
      // Check whether the function can return without sret-demotion.
      SmallVector<ISD::OutputArg, 4> Outs;
      GetReturnInfo(F.getCallingConv(), F.getReturnType(), F.getAttributes(),
          Outs, *TLI, TD);
      assert(TLI->CanLowerReturn(F.getCallingConv(),
            const_cast<MachineFunction&>(MF), F.isVarArg(), Outs, Ctx));

      // Set up the incoming argument description vector.
      unsigned Idx = 1;
      for (Function::const_arg_iterator I = F.arg_begin(), E = F.arg_end();
          I != E; ++I, ++Idx) {
        SmallVector<EVT, 4> ValueVTs;
        ComputeValueVTs(*TLI, TD, I->getType(), ValueVTs);
        bool isArgValueUsed = !I->use_empty();
        for (unsigned Value = 0, NumValues = ValueVTs.size();
            Value != NumValues; ++Value) {
          EVT VT = ValueVTs[Value];
          Type *ArgTy = VT.getTypeForEVT(Ctx);
          ISD::ArgFlagsTy Flags;
          unsigned OriginalAlignment =
            TD.getABITypeAlignment(ArgTy);

          if (F.getAttributes().hasAttribute(Idx, Attribute::ZExt))
            Flags.setZExt();
          if (F.getAttributes().hasAttribute(Idx, Attribute::SExt))
            Flags.setSExt();
          if (F.getAttributes().hasAttribute(Idx, Attribute::InReg))
            Flags.setInReg();
          if (F.getAttributes().hasAttribute(Idx, Attribute::StructRet))
            Flags.setSRet();
          if (F.getAttributes().hasAttribute(Idx, Attribute::ByVal)) {
            Flags.setByVal();
            PointerType *Ty = cast<PointerType>(I->getType());
            Type *ElementTy = Ty->getElementType();
            Flags.setByValSize(TD.getTypeAllocSize(ElementTy));
            // For ByVal, alignment should be passed from FE.  BE will guess if
            // this info is not there but there are cases it cannot get right.
            unsigned FrameAlign;
            if (F.getParamAlignment(Idx))
              FrameAlign = F.getParamAlignment(Idx);
            else
              FrameAlign = TLI->getByValTypeAlignment(ElementTy, TD);
            Flags.setByValAlign(Align(FrameAlign));
          }
          if (F.getAttributes().hasAttribute(Idx, Attribute::Nest))
            Flags.setNest();
          Flags.setOrigAlign(Align(OriginalAlignment));

          EVT RegisterVT = TLI->getRegisterType(Ctx, VT);
          unsigned NumRegs = TLI->getNumRegisters(Ctx, VT);
          for (unsigned i = 0; i != NumRegs; ++i) {
            // TODO check if the VT arguments are correct.. no docs around.
            ISD::InputArg MyFlags(Flags, RegisterVT, VT, isArgValueUsed,
                Idx-1, i*RegisterVT.getStoreSize());
            if (NumRegs > 1 && i == 0)
              MyFlags.Flags.setSplit();
            // if it isn't first piece, alignment must be 1
            else if (i > 0)
              MyFlags.Flags.setOrigAlign(Align(1));
            Ins.push_back(MyFlags);
          }
        }
      }

      /////////////////
      // see
      // PatmosTargetLowering::LowerCCCArguments() in PatmosISelLowering.cpp
      /////////////////
      // Assign locations to all of the incoming arguments.
      SmallVector<CCValAssign, 16> ArgLocs;
      CCState CCInfo(F.getCallingConv(), false/*isVarArg*/,
                     const_cast<MachineFunction&>(MF), ArgLocs, Ctx);
      CCInfo.AnalyzeFormalArguments(Ins, CC_Patmos);
      /////////////////


      // Lowered arguments are computed, now we can match them back again
      // and do the actual exporting

      unsigned FAIdx = 0, // formal argument index
               LAIdx = 0; // lowered argument index

      // we have a 1-1 mapping of Ins to ArgLocs, we access them via LAIdx
      for (Function::const_arg_iterator I = F.arg_begin(), E = F.arg_end();
          I != E; ++I, ++FAIdx) {

        // Place LAIdx to the position where the lowered arguments for the
        // corresponding formal argument start
        while(Ins[LAIdx].OrigArgIndex < FAIdx) LAIdx++;

        if (isa<IntegerType>(I->getType())) {
          // being an integer, this might be an interesting parameter

          // TODO Do we need to test special flags (zero-/sign-extend)
          //      and output more information?
          // Note that we start with the low-part registers first
          assert(TLI->getValueType(TD, I->getType()).isSimple());

          LLVM_DEBUG( dbgs() << FAIdx << " " << *I << ": [" );

          // FIXME
          // we don't add it immediately to PMF, as we only support
          // arguments in registers at this point
          std::string ArgName = I->getNameOrAsOperand();
          yaml::Argument *Arg = new yaml::Argument(ArgName, FAIdx);
          bool allInRegs = true;

          // get all registers with OrigArgIndex == FAIdx
          for( ; LAIdx < Ins.size() && Ins[LAIdx].OrigArgIndex == FAIdx;
              LAIdx++) {
            CCValAssign &VA = ArgLocs[LAIdx];

            if (!VA.isRegLoc()) {
              // no support yet, bailout
              allInRegs = false;
              break;
            }

            // we prefer the name of the register as is printed in assembly
            Arg->addReg(PatmosInstPrinter::getRegisterName(VA.getLocReg()));

            LLVM_DEBUG( dbgs() <<  TM.getRegisterInfo()->getName(VA.getLocReg())
		          << " " );
          }

          LLVM_DEBUG( dbgs() <<  "]\n" );

          if (allInRegs) {
            PMF->addArgument(Arg);
          } else {
            delete Arg;
          }
        }
      }
    }

    void PatmosMachineExport::
    exportInstruction(MachineFunction &MF,
                      yaml::MachineInstruction *I,
                      const MachineInstr *Instr,
                      bool BundledWithPred) {

      PatmosStackCacheAnalysisInfo *SCA =
       &P.getAnalysis<PatmosStackCacheAnalysisInfo>();

      // Export the argument of reserve, ensure, free instructions
      if (Instr->getOpcode() == Patmos::SENSi ||
          Instr->getOpcode() == Patmos::SRESi ||
          Instr->getOpcode() == Patmos::SFREEi) {
        I->StackCacheArg = Instr->getOperand(2).getImm();
      }
      // Export the worst-case spill and fill counts (if analysis available)
      if (SCA->isValid()) {
        if (Instr->getOpcode() == Patmos::SENSi) {
          PatmosStackCacheAnalysisInfo::FillSpillCounts::iterator it =
            SCA->Ensures.find(Instr);
          assert(it != SCA->Ensures.end());
          I->StackCacheFill = it->second;
        } else if (Instr->getOpcode() == Patmos::SRESi) {
          PatmosStackCacheAnalysisInfo::FillSpillCounts::iterator it =
            SCA->Reserves.find(Instr);
          assert(it != SCA->Reserves.end());
          I->StackCacheSpill = it->second;
        }
      }

      if (!Instr->isInlineAsm() && (Instr->mayLoad() || Instr->mayStore())) {
        const PatmosInstrInfo *PII =
          static_cast<const PatmosInstrInfo*>(TM.getInstrInfo());
        switch (PII->getMemType(*Instr)) {
          case PatmosII::MEM_S: I->MemType = "stack";  break;
          case PatmosII::MEM_L: I->MemType = "local";  break;
          case PatmosII::MEM_M: I->MemType = "memory"; break;
          case PatmosII::MEM_C: I->MemType = "cache";  break;
        }
      }
      return PMLMachineExport::exportInstruction(MF, I, Instr, BundledWithPred);
    }


    void PatmosMachineExport::exportSubfunctions(MachineFunction &MF,
                                                 yaml::PMLMachineFunction *PMF)
    {
      // TODO use some PML mapping function to get the unique label for MBBs
      yaml::Subfunction *S = new yaml::Subfunction(MF.begin()->getNumber());
      const PatmosMachineFunctionInfo *PMFI =
                                        MF.getInfo<PatmosMachineFunctionInfo>();

      for (MachineFunction::iterator bb = MF.begin(), be = MF.end(); bb != be;
           bb++)
      {
        if (bb != MF.begin() && PMFI->isMethodCacheRegionEntry(&*bb)) {
          PMF->addSubfunction(S);
          S = new yaml::Subfunction(bb->getNumber());
        }
        S->addBlock(bb->getNumber());
      }
      PMF->addSubfunction(S);
    }

    void PatmosMachineExport::exportLoopInfo(MachineFunction &MF,
                                             yaml::PMLDoc<yaml::PMLMachineFunction,yaml::UnsignedValue> &YDoc,
                                             MachineLoop *Loop)
    {
      // Export user loop bounds for single-path functions since we
      // know that they are the exact loop bounds.
      if (!PatmosSinglePathInfo::isEnabled(MF)) return;

      // scan the header for loopbound info
      MachineBasicBlock *Header = Loop->getHeader();
      auto LoopBound = getLoopBounds(Header);
      if (LoopBound) {
        auto *FF = new yaml::FlowFact<yaml::UnsignedValue>(yaml::level_machinecode);

        FF->setLoopScope(MF.getFunctionNumber(),
                          Header->getNumber());

        yaml::ProgramPoint *Block =
          yaml::ProgramPoint::CreateBlock(utostr(MF.getFunctionNumber()),
              utostr(Header->getNumber()));

        FF->addTermLHS(Block, 1LL);
        FF->RHS = LoopBound->second;
        FF->Comparison = yaml::cmp_less_equal;
        FF->Origin = "user.bc";
        FF->Classification = "loop-local";

        YDoc.addFlowFact(FF);
      }
    }
} // end namespace llvm

