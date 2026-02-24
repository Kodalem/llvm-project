add_llvm_component_library(LLVMPatmosSinglePath
  PatmosSinglePathInfo.cpp
  PatmosSPClone.cpp
  PatmosSPMark.cpp
  PatmosSPPrepare.cpp
  PatmosSPBundling.cpp
  DataCacheAccessElimination.cpp
  PatmosSPReduce.cpp
  PreRegallocReduce.cpp
  Linearizer.cpp
  RAInfo.cpp
  SPScope.cpp
  SPScheduler.cpp
  PredicateDefinition.cpp
  OppositePredicateCompensation.cpp
  MemoryAccessNormalization.cpp
  ConstantLoopDominators.cpp
  LoopCountInsert.cpp
  VirtualizePredicates.cpp
  EquivalenceClasses.cpp
  InstructionCounter.cpp
  FCFGPostDom.cpp
  
  LINK_COMPONENTS
  PatmosInfo 
  MC 
  Support
 
  ADD_TO_COMPONENT
  Patmos
  
  )