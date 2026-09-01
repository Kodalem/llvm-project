; REQUIRES: patmos-expensive-tests
; This is the stress version. The default suite runs
; long_4-align_1-div_smoke.ll with the same alignment/remainder class and a
; smaller count; enable patmos-expensive-tests for the original large-count coverage.
; RUN: env MEMSET_COUNT=41 MEMSET_ALLOC_COUNT=41 MEMSET_PTR_INC=0 %memset_check_i32_main 0=0 1=41 2=82 6=246
; RUN: env MEMSET_COUNT=41 MEMSET_ALLOC_COUNT=41 MEMSET_PTR_INC=0 %memset_check_i32_indirect 0=0 1=41 2=82 6=246
; RUN: env MEMSET_COUNT=41 MEMSET_ALLOC_COUNT=41 MEMSET_PTR_INC=0 %memset_check_i64_main 0=0 1=41 2=82 6=246
; RUN: env MEMSET_COUNT=41 MEMSET_ALLOC_COUNT=41 MEMSET_PTR_INC=0 %memset_check_i64_indirect 0=0 1=41 2=82 6=246
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
;
; Tests can use llvm.memset without standard library 'memset' for:
; * High length
; * 4-Aligned start
; * 1-divisable length
;
;//////////////////////////////////////////////////////////////////////////////////////////////////
