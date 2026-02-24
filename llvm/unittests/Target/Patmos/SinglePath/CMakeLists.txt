set(LLVM_LINK_COMPONENTS
  PatmosSinglePath
  )

set(PatmosSource
  PredicatedBlockTest.cpp
  MemoryAccessAnalysisTest.cpp
  ConstantLoopDominatorAnalysisTest.cpp
  ListSchedulerTest.cpp
  )

add_llvm_unittest(SinglePathTests
  ${PatmosSource}
  )
