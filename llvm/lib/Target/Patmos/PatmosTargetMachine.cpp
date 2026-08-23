//===-- PatmosTargetMachine.cpp - Define TargetMachine for Patmos ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Top-level implementation for the Patmos target.
//
//===----------------------------------------------------------------------===//

#include "Patmos.h"
#include "PatmosTargetMachine.h"
#include "PatmosMachineFunctionInfo.h"
#include "SinglePath/PatmosSinglePathInfo.h"
#include "PatmosSchedStrategy.h"
#include "PatmosStackCacheAnalysis.h"
#include "TargetInfo/PatmosTargetInfo.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Pass.h"
#include "llvm/Support/MemoryBuffer.h"

namespace llvm {

    extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializePatmosTarget() {
        // Register the target.
        RegisterTargetMachine<PatmosTargetMachine> X(getThePatmosTarget());
    }

    static ScheduleDAGInstrs *createPatmosVLIWMachineSched(MachineSchedContext *C) {
        // TODO instead of the generic ScheduleDAGMI, we might want to use a different
        // scheduler that allows for VLIW bundling or something, similar to the
        // PatmosPostRAScheduler. Should still be split into a generic ScheduleDAG
        // scheduler and a specialised PatmosSchedStrategy.
        ScheduleDAGMI *PS = new ScheduleDAGMI(C, std::make_unique<PatmosVLIWSchedStrategy>(), false);
        return PS;
    }

    static MachineSchedRegistry
    SchedCustomRegistry("patmos", "Run Patmos's custom scheduler",
                        createPatmosVLIWMachineSched);


    namespace {
        /// EnableStackCacheAnalysis - Option to enable the analysis of Patmos' stack
        /// cache usage.
        static cl::opt<bool> EnableStackCacheAnalysis(
          "mpatmos-enable-stack-cache-analysis",
          cl::init(false),
          cl::desc("Enable the Patmos stack cache analysis."),
          cl::Hidden);
        static cl::opt<bool> DisableIfConverter(
            "mpatmos-disable-ifcvt",
            cl::init(false),
            cl::desc("Disable if-converter for Patmos."),
            cl::Hidden);
        static cl::opt<std::string> SerializeMachineCode(
            "mserialize-pml",
            cl::init(""),
            cl::desc("Export PML specification of generated machine code to FILE"));
        static cl::list<std::string>SerializeRoots("mserialize-pml-functions",
           cl::desc("Export only given method (default: 'main') and those reachable from them"),
           cl::CommaSeparated);
        static cl::opt<bool> SerializeAllFunctions(
            "mserialize-pml-all",
            cl::init(false),
            cl::desc("Export PML machine-function entries for every function "
                     "defined in the module, ignoring -mserialize-pml-functions "
                     "reachability roots. Useful for compiling library crates "
                     "(e.g. compiler_builtins from Rust) on their own, where none of the "
                     "library's own functions are reachable from a 'main'/'_start' "
                     "root within that compilation unit."));





        /// Patmos Code Generator Pass Configuration Options.
        class PatmosPassConfig : public TargetPassConfig {
        private:

        public:
            PatmosPassConfig(PatmosTargetMachine &TM, PassManagerBase &PM)
             : TargetPassConfig(TM, PM)
            {
                if( PatmosSinglePathInfo::isConstant() && !PatmosSinglePathInfo::isEnabled() ) {
                    report_fatal_error("The 'mpatmos-enable-cet' option "
                        "requires the 'mpatmos-singlepath' option to also be set.");
                }
                if( !SerializeRoots.empty() && SerializeMachineCode=="" ){
                    report_fatal_error("The 'mserialize-pml-functions' option "
                        "requires the 'mserialize-pml' option to also be set.");
                }

                // Enable preRA MI scheduler.
                if (TM.getSubtargetImpl()->usePreRAMIScheduler(getOptLevel())) {
                    enablePass(&MachineSchedulerID);
                }
                if (TM.getSubtargetImpl()->usePatmosPostRAScheduler(getOptLevel())) {
                    initializePatmosPostRASchedulerPass(*PassRegistry::getPassRegistry());
                    substitutePass(&PostRASchedulerID, &PatmosPostRASchedulerID);
                }
            }

            PatmosTargetMachine &getPatmosTargetMachine() const {
                return getTM<PatmosTargetMachine>();
            }

            const PatmosSubtarget &getPatmosSubtarget() const {
                return *getPatmosTargetMachine().getSubtargetImpl();
            }

            ScheduleDAGInstrs *
            createMachineScheduler(MachineSchedContext *C) const {
                return createPatmosVLIWMachineSched(C);
            }

            ScheduleDAGInstrs *
            createPostMachineScheduler(MachineSchedContext *C) const {
                llvm_unreachable("unimplemented");
            }

            bool addInstSelector() override {
                addPass(createPatmosISelDag(getPatmosTargetMachine(), getOptLevel()));
                return false;
            }

            void addPreISelIntrinsicLowering() override {
                // LLVM22 lowers memset/memcpy very early in the generic pipeline.
                // Ensure single-path attribution exists first, so intrinsic elimination
                // can reliably diagnose variable-length mem intrinsics in SP functions.
                if (PatmosSinglePathInfo::isEnabled()) {
                    addPass(createPatmosSPClonePass());
                }

                // Then run Patmos' intrinsic elimination before generic pre-isel
                // intrinsic lowering, so we preserve Patmos loop-bound semantics.
                addPass(createPatmosIntrinsicEliminationPass());
            }

            //
            /// addPreISelPasses - This method should add any "last minute" LLVM->LLVM
            /// passes (which are run just before instruction selector).
            bool addPreISel() override {
                if (PatmosSinglePathInfo::isEnabled()) {
                    // Single-path transformation requires a single exit node
                    // addPass(createUnifyFunctionExitNodesPass()); // Removed in recent LLVM versions!
                    // Single-path transformation currently cannot deal with
                    // switch/jumptables -> lower them to ITEs
                    addPass(createLowerSwitchPass());
                }

                return PatmosSinglePathInfo::isEnabled();
            }

            /// addPreRegAlloc - This method may be implemented by targets that want to
            /// run passes immediately before register allocation. This should return
            /// true if -print-machineinstrs should print after these passes.
            void addPreRegAlloc() override {
                // For -O0, add a pass that removes dead instructions to avoid issues
                // with spill code in naked functions containing function calls with
                // unused return values.
                // Note: addPass no longer accepts a second argument in newer LLVM versions.
                // The do/while loop with Changed was removed as PatmosEmptyBlockElimID
                // no longer exists and addPass signature changed.
                if (getOptLevel() == CodeGenOptLevel::None) {
                    addPass(&DeadMachineInstructionElimID);
                }

                if (PatmosSinglePathInfo::isConstant()) {
                    addPass(createDataCacheAccessEliminationPass(getPatmosTargetMachine()));
                }
                if (PatmosSinglePathInfo::isEnabled()) {
                    addPass(createPatmosSPMarkPass(getPatmosTargetMachine()));
                    if(!PatmosSinglePathInfo::useNewSinglePathTransform()) {
                        addPass(createPatmosSinglePathInfoPass(getPatmosTargetMachine()));
                        addPass(createPatmosSPPreparePass(getPatmosTargetMachine()));
                    }
                    if (PatmosSinglePathInfo::isConstant()) {
                        addPass(createPatmosConstantLoopDominatorsPass());
                        addPass(createMemoryAccessNormalizationPass(getPatmosTargetMachine()));
                    }
                    if(PatmosSinglePathInfo::useNewSinglePathTransform()) {
                        addPass(createLoopCountInsert(getPatmosTargetMachine()));
                    }
                }
            }

            /// This method may be implemented by targets that want to run passes after
            /// register allocation pass pipeline but before prolog-epilog insertion.
            void addPostRegAlloc() override {
                if(PatmosSinglePathInfo::isEnabled() && PatmosSinglePathInfo::useNewSinglePathTransform()) {
                    // The previous register allocation allocated the predicate registers too.
                    // We returns those predicate registers to virtual registers so that they
                    // play nice with the rest of the predicates from the single-path management.
                    addPass(createVirtualizePredicates(getPatmosTargetMachine()));

                    // At this point there are still register copies that haven't been converted to patmos instructions.
                    // (in addition to the ones added by VirtualizePredicates).
                    // So, Optimize as many of them away as possible and
                    // convert them to patmos instruction such that they can be predicated.
                    // (COPY is a pseudo-instruction and can't be predicated)
                    addPass(&RegisterCoalescerID); // Optimize copies
                    addPass(&ExpandPostRAPseudosID); // Convert to patmos instructions

                    // Calculate equivalence classes and assign abstract predicates to each
                    addPass(createEquivalenceClassesPass());

                    // Assign predicates to instructions and initialize predicate definitions
                    addPass(createPreRegallocReduce(getPatmosTargetMachine()));

                    addPass(createPatmosSinglePathInfoPass(getPatmosTargetMachine()));
                    if (PatmosSinglePathInfo::isConstant()) {
                        addPass(createPatmosConstantLoopDominatorsPass());
                        addPass(createOppositePredicateCompensationPass(getPatmosTargetMachine()));
                    }

                    // Linearize block order and control flow and merge blocks
                    addPass(createSinglePathLinearizer(getPatmosTargetMachine()));

                    ///////////////
                    // The following is copied from TargetPassConfig::addOptimizedRegAlloc()
                    // with unneeded parts commented out

                    // Note: addPass no longer accepts a second argument in newer LLVM versions.
                    addPass(&MachineLoopInfoID);
                    //			addPass(&PHIEliminationID, false);

                    //			addPass(&TwoAddressInstructionPassID, false);
                    addPass(&RegisterCoalescerID);

                    // The machine scheduler may accidentally create disconnected components
                    // when moving subregister definitions around, avoid this by splitting them to
                    // separate vregs before. Splitting can also improve reg. allocation quality.
                    //			addPass(&RenameIndependentSubregsID);

                    // PreRA instruction scheduling.
                    //			addPass(&MachineSchedulerID);

                    if (addRegAssignAndRewriteOptimized()) {
                        // Perform stack slot coloring and post-ra machine LICM.
                        //
                        // FIXME: Re-enable coloring with register when it's capable of adding
                        // kill markers.
                        addPass(&StackSlotColoringID);

                        // Allow targets to expand pseudo instructions depending on the choice of
                        // registers before MachineCopyPropagation.
                        //				addPostRewrite();

                        // Copy propagate to forward register uses and try to eliminate COPYs that
                        // were not coalesced.
                        addPass(&MachineCopyPropagationID);

                        // Run post-ra machine LICM to hoist reloads / remats.
                        //
                        // FIXME: can this move into MachineLateOptimization?
                        addPass(&MachineLICMID);
                    }
                    // End of copy from TargetPassConfig::addOptimizedRegAlloc()
                    ///////////////
                }
            }

            /// addPreSched2 - This method may be implemented by targets that want to
            /// run passes after prolog-epilog insertion and before the second instruction
            /// scheduling pass.  This should return true if -print-machineinstrs should
            /// print after these passes.
            void addPreSched2() override {

                if (PatmosSinglePathInfo::isEnabled()) {
                    if(!PatmosSinglePathInfo::useNewSinglePathTransform()) {
                        addPass(createPatmosSinglePathInfoPass(getPatmosTargetMachine()));
                        if (PatmosSinglePathInfo::isConstant()) {
                            addPass(createPatmosConstantLoopDominatorsPass());
                            addPass(createOppositePredicateCompensationPass(getPatmosTargetMachine()));
                        }
                        addPass(createPatmosSPBundlingPass(getPatmosTargetMachine()));
                        addPass(createPatmosSPReducePass(getPatmosTargetMachine()));
                    }
                    addPass(createSPSchedulerPass(getPatmosTargetMachine()));
                } else {
                    // Note: CodeGenOpt::None was renamed to CodeGenOptLevel::None in newer LLVM.
                    if (getOptLevel() != CodeGenOptLevel::None && !DisableIfConverter) {
                        addPass(&IfConverterID);
                        // If-converter might create unreachable blocks (bug?), need to be
                        // removed before function splitter
                        addPass(&UnreachableMachineBlockElimID);
                    }
                }

                // this is pseudo pass that may hold results from SC analysis
                // (currently for PML export)
                addPass(createPatmosStackCacheAnalysisInfo(getPatmosTargetMachine()));

                if (EnableStackCacheAnalysis) {
                    addPass(createPatmosStackCacheAnalysis(getPatmosTargetMachine()));
                }
            }

            void addBlockPlacement() override {
                // We leave this function empty to avoid LLVM's default
                // probabilistic block placement pass, as it seems to
                // mess with terminating instructions on some blocks,
                // resulting in invalid branch targets.
            }

            /// addPreEmitPass - This pass may be implemented by targets that want to run
            /// passes immediately before machine code is emitted.  This should return
            /// true if -print-machineinstrs should print out the code after the passes.
            void addPreEmitPass() override {

                // Post-RA MI Scheduler does bundling and delay slots itself. Otherwise,
                // add passes to handle them.
                if (!getPatmosSubtarget().usePatmosPostRAScheduler(getOptLevel())) {
                    addPass(createPatmosDelaySlotFillerPass(getPatmosTargetMachine(),
                                                        getOptLevel() == CodeGenOptLevel::None));
                }

                // All passes below this line must handle delay slots and bundles
                // correctly.

                if (getPatmosSubtarget().hasMethodCache()) {
                    addPass(createPatmosFunctionSplitterPass(getPatmosTargetMachine()));
                }

                addPass(createPatmosDelaySlotKillerPass(getPatmosTargetMachine()));

                addPass(createPatmosEnsureAlignmentPass(getPatmosTargetMachine()));

                // Serialize machine code
                if (!SerializeMachineCode.empty()) {
                    std::string empty("");
                    if(SerializeRoots.empty()){
                        SerializeRoots.push_back("main");
                    }

                    addPass(createPatmosModuleExportPass(getPatmosTargetMachine(),
                        SerializeMachineCode, empty, SerializeRoots,
                        SerializeAllFunctions));
                }
                if(PatmosSinglePathInfo::isEnabled()) {
                    addPass(createSinglePathInstructionCounter(getPatmosTargetMachine()));
                }
            }
        };
    } // namespace

    // Note: Optional was replaced by std::optional and hasValue() by has_value() in newer LLVM.
    static Reloc::Model getEffectiveRelocModel(bool JIT,
                                               std::optional<Reloc::Model> RM) {
        if (!RM.has_value() || JIT)
            return Reloc::Static;
        return *RM;
    }

    PatmosTargetMachine::PatmosTargetMachine(const Target &T,
                                             const Triple &TT,
                                             StringRef CPU,
                                             StringRef FS,
                                             const TargetOptions &Options,
                                             std::optional<Reloc::Model> RM,
                                             std::optional<CodeModel::Model> CM,
                                             CodeGenOptLevel L, bool JIT)
      // Note: Base class changed from LLVMTargetMachine to CodeGenTargetMachineImpl in newer LLVM.
      : CodeGenTargetMachineImpl(
          T,
          // Keep this in sync with clang/lib/Basic/Targets.cpp and
          // compiler-rt/lib/patmos/*.ll
          // Note: Both ABI and Preferred Alignment must be 32bit for all supported
          // types, backend does not support different stack alignment.
          "E-S32-p:32:32:32-i8:8:8-i16:16:16-i32:32:32-i64:32:32-f64:32:32-a0:0:32-s0:32:32-v64:32:32-v128:32:32-n32",
          TT, CPU, FS, Options, getEffectiveRelocModel(JIT, RM), getEffectiveCodeModel(CM, CodeModel::Small), L),
          TLOF(std::make_unique<PatmosTargetObjectFile>()),
          Subtarget(TT, CPU, FS, *this, L)
    {
        // Ensure temporary labels are preserved for later passes that rely on
        // creating and referring to temporary labels in assembly. Set this in the
        // TargetMachine during construction rather than from the AsmPrinter, since
        // the Options member is protected in the base TargetMachine class and
        // should be configured at creation time.
        this->Options.MCOptions.MCSaveTempLabels = true;

        initAsmInfo();
    }

    TargetPassConfig *PatmosTargetMachine::createPassConfig(PassManagerBase &PM) {
        return new PatmosPassConfig(*this, PM);
    }

    TargetTransformInfo
    PatmosTargetMachine::getTargetTransformInfo(const Function &F) const {
        // Keep a conservative default until a Patmos-specific TTI implementation is added.
        return TargetTransformInfo(F.getDataLayout());
    }

    MachineFunctionInfo *PatmosTargetMachine::createMachineFunctionInfo(
        BumpPtrAllocator &Allocator, const Function &F,
        const TargetSubtargetInfo *STI) const {
        return PatmosMachineFunctionInfo::create<PatmosMachineFunctionInfo>(
            Allocator, F, STI);
    }

    void PatmosTargetMachine::registerPassBuilderCallbacks(PassBuilder &PB) {
        (void)PB;
    }
} // end namespace llvm