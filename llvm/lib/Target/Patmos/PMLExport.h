//===- lib/CodeGen/PMLExport.h -----------------------------------------===//
//
//               YAML definitions for exporting LLVM datastructures
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CODEGEN_PML_EXPORT_H_
#define LLVM_CODEGEN_PML_EXPORT_H_

#include "PML.h"
#include "PatmosTargetMachine.h"
#include "llvm/IR/Module.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/CodeGen/MachineLoopInfo.h"

#include <list>

/////////////////
/// PML Export //
/////////////////

namespace llvm {

  class MachineLoop;

  /// Provides information about machine instructions, can be overloaded for
  /// specific targets.
  class PMLInstrInfo {
  public:
    virtual ~PMLInstrInfo() {}

    typedef std::vector<StringRef>                StringList;
    typedef const std::vector<MachineBasicBlock*> MBBList;
    typedef std::vector<MachineFunction*>         MFList;

    // TODO merge getCalleeNames and getCallees somehow (return struct that
    // contains both names and MFs)

    /// getCalleeNames - get the names of the possible called functions.
    /// If a callee has no name, it is omitted.
    virtual StringList getCalleeNames(MachineFunction &Caller,
                                      const MachineInstr *Instr);

    /// getCallees - get possible callee functions for a call.
    /// If the name of a callee is known but not in this module, it is omitted.
    virtual MFList getCallees(const Module &M, MachineModuleInfo &MMI,
                              MachineFunction &MF, const MachineInstr *Instr);

    virtual MFList getCalledFunctions(const Module &M,
                                      MachineModuleInfo &MMI,
                                      MachineFunction &MF);

    virtual MBBList getBranchTargets(MachineFunction &MF,
                                     const MachineInstr *Instr);

    /// get number of delay slots for this instruction, if any
    virtual unsigned getBranchDelaySlots(const MachineInstr *Instr) {
      return 0;
    }

    /// get number of bytes this instruction takes
    virtual int getSize(const MachineInstr *Instr);
  };

  /// Base class for all exporters
  class PMLExport {

  public:
    PMLExport() {}

    virtual ~PMLExport() {}

    virtual void initialize(const Module &M) {}

    virtual void finalize(const Module &M) {}

    virtual void serialize(MachineFunction &MF) =0;

    virtual void writeOutput(yaml::Output *Output) =0;
  };


  // --------------- Standard exporters --------------------- //

  // TODO we could factor out the code to generate the object to export and
  // do the Doc related stuff separately, if needed for efficiency reasons, or
  // to update existing yaml-docs.

  class PMLBitcodeExport : public PMLExport {
  private:

    yaml::PMLDoc<yaml::BitcodeFunction,yaml::StringValue> YDoc;
    Pass &P;

    yaml::FlowFact<yaml::StringValue> *createLoopFact(const BasicBlock *BB, yaml::UnsignedValue RHS,
                                   bool UserAnnot = false) const;

  public:
    PMLBitcodeExport(TargetMachine &TM, ModulePass &mp)
    : YDoc("bitcode-functions", TM.getTargetTriple().getTriple()), P(mp) {}

    virtual ~PMLBitcodeExport() { }

    // initialize module-level information
    virtual void initialize(const Module &M) { }

    // export bitcode function
    virtual void serialize(MachineFunction &MF);

    // export module-level information during finalize()
    virtual void finalize(const Module &M) { }

    virtual void writeOutput(yaml::Output *Output) {
      auto *DocPtr = &YDoc; *Output << DocPtr;
    }

    yaml::PMLDoc<yaml::BitcodeFunction,yaml::StringValue>& getPMLDoc() { return YDoc; }

    virtual bool doExportInstruction(const Instruction* Instr) {
      return true;
    }

    virtual yaml::StringValue getOpcode(const Instruction *Instr);

    virtual void exportInstruction(yaml::Instruction* I,
                                   const Instruction* II);
  };



  class PMLMachineExport : public PMLExport {
  private:
    yaml::PMLDoc<yaml::PMLMachineFunction,yaml::UnsignedValue> YDoc;

    PMLInstrInfo *PII;

    yaml::ValueFact *createMemGVFact(const MachineInstr *MI,
                                     yaml::MachineInstruction *I,
                                     std::set<const GlobalValue*> &GVs) const;
  protected:
    Pass &P;
    PatmosTargetMachine &TM;

    const TargetInstrInfo *TII;

  public:
    PMLMachineExport(PatmosTargetMachine &tm, ModulePass &mp, const TargetInstrInfo *TII,
                     PMLInstrInfo *pii = 0)
      : YDoc("machine-functions", tm.getTargetTriple().getTriple()), P(mp), TM(tm), TII(TII)
    {
      // TODO needs to be deleted properly!
      PII = pii ? pii : new PMLInstrInfo();
    }

    virtual ~PMLMachineExport() {}

    virtual void serialize(MachineFunction &MF);

    virtual void writeOutput(yaml::Output *Output) {
    	yaml::PMLDoc<yaml::PMLMachineFunction,yaml::UnsignedValue> *DocPtr = &YDoc;
    	*Output << DocPtr;
    }

    yaml::PMLDoc<yaml::PMLMachineFunction,yaml::UnsignedValue>& getPMLDoc() { return YDoc; }

    virtual bool doExportInstruction(const MachineInstr *Instr) {
      return true;
    }

    virtual yaml::StringValue getOpcode(const MachineInstr *Instr);

    virtual void exportInstruction(MachineFunction &MF,
                                   yaml::MachineInstruction *I,
                                   const MachineInstr *Instr,
                                   bool BundledWithPred);
    virtual void exportCallInstruction(MachineFunction &MF,
                                   yaml::MachineInstruction *I,
                                   const MachineInstr *Instr);
    virtual void exportBranchInstruction(MachineFunction &MF,
                                   yaml::MachineInstruction *I,
                                   const MachineInstr *Instr);
    virtual void exportMemInstruction(MachineFunction &MF,
                                   yaml::MachineInstruction *I,
                                   const MachineInstr *Instr);

    virtual void exportArgumentRegisterMapping(
                                      yaml::PMLMachineFunction *F,
                                      const MachineFunction &MF);

    virtual void exportSubfunctions(MachineFunction &MF,
                                    yaml::PMLMachineFunction *PMF) { }
    virtual void exportLoopInfo(MachineFunction &MF,
                                yaml::PMLDoc<yaml::PMLMachineFunction,yaml::UnsignedValue> &YDoc,
                                MachineLoop *Loop) { }
  };

  class PMLRelationGraphExport : public PMLExport {
  private:
	// We use BitcodeFunction as placeholder since its not used
    yaml::PMLDoc<yaml::BitcodeFunction,yaml::StringValue> YDoc;
    Pass &P;

  public:
    PMLRelationGraphExport(TargetMachine &TM, ModulePass &mp)
      : YDoc("ERROR(PMLRelationGraphExport)", TM.getTargetTriple().getTriple()), P(mp) {}

    virtual ~PMLRelationGraphExport() {}

    /// Build the Control-Flow Relation Graph connection between
    /// machine code and bitcode
    virtual void serialize(MachineFunction &MF);

    virtual void writeOutput(yaml::Output *Output) {
    	auto *DocPtr = &YDoc; *Output << DocPtr;
    }

    yaml::PMLDoc<yaml::BitcodeFunction,yaml::StringValue>& getPMLDoc() { return YDoc; }

  private:

    /// Generate (heuristic) MachineBlock->EventName
    /// and IR-Block->EventName maps
    /// (1) if all forward-CFG predecessors of (MBB originating from BB) map to
    ///     no or a different IR block, MBB generates a BB event.
    /// (2) if there is a MBB generating a event BB, the basic block BB also
    ///     generates this event
    void buildEventMaps(MachineFunction &MF,
                        std::map<const BasicBlock*,StringRef> &BitcodeEventMap,
                        std::map<MachineBasicBlock*,StringRef> &MachineEventMap,
                        std::set<StringRef> &TabuList);

    class BackedgeInfo {
    private:
      MachineLoopInfo &MLI;
    public:
      BackedgeInfo(MachineLoopInfo &mli) : MLI(mli) {}
      ~BackedgeInfo() {}
      /// Check whether Source -> Target is a back-edge
      bool isBackEdge(MachineBasicBlock *Source, MachineBasicBlock *Target);
    };

  };

  // ---------------------- Export Passes ------------------------- //


  // TODO this pass is currently implemented to work as machine-code module
  // pass. It should either support running on bitcode only as well, or
  // implement another pass for that.
  class PMLModuleExportPass : public ModulePass {

    static char ID;

    typedef std::vector<PMLExport*>        ExportList;
    typedef std::vector<std::string>       StringList;
    typedef std::list<MachineFunction*>    MFQueue;
    typedef std::set<MachineFunction*>     MFSet;

    ExportList Exporters;

    PMLInstrInfo *PII;

    StringRef   OutFileName;
    std::string BitcodeFile;
    StringList  Roots;
    bool        SerializeAll;

    MFSet   FoundFunctions;
    MFQueue Queue;

  protected:
    /// Constructor to be used by sub-classes, passes the pass ID to the super
    /// class. You need to setup a PMLInstrInfo using setPMLInstrInfo before
    /// using the exporter.
    PMLModuleExportPass(char &ID, TargetMachine &TM, StringRef filename,
                        ArrayRef<std::string> roots, bool SerializeAll);

    /// Set the PMLInstrInfo to be used. Takes ownership over the
    /// InstrInfo object.
    void setPMLInstrInfo(PMLInstrInfo *pii) { PII = pii; }

  public:
    /// Construct a new PML export pass. The pass will take ownership of the
    /// given PMLInstrInfo.
    PMLModuleExportPass(TargetMachine &TM, StringRef filename,
                        ArrayRef<std::string> roots, PMLInstrInfo *pii,
                        bool SerializeAll);

    ~PMLModuleExportPass() override {
      while(!Exporters.empty()) {
        delete Exporters.back();
        Exporters.pop_back();
      }
      if (PII) delete PII;
    }

    PMLInstrInfo *getPMLInstrInfo() { return PII; }

    /// Add an exporter to the pass. Exporters will be deleted when this pass
    /// is deleted.
    void addExporter(PMLExport *PE) { Exporters.push_back(PE); }

    void writeBitcode(std::string& bitcodeFile) { BitcodeFile = bitcodeFile; }

    StringRef getPassName() const override {
      return "YAML/PML Module Export";
    }

    void getAnalysisUsage(AnalysisUsage &AU) const override;

    bool runOnModule(Module &M) override;

  protected:

    bool doInitialization(Module &M) override;

    bool doFinalization(Module &M) override;

    void addCalleesToQueue(const Module &M, MachineModuleInfo &MMI,
                           MachineFunction &MF);

    void addToQueue(const Module &M, MachineModuleInfo &MMI,
                    std::string FnName);

    void addToQueue(MachineFunction &MF);

  };

} // end namespace llvm

#endif
