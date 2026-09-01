; REQUIRES: patmos-expensive-tests
; This is the stress version. The default suite runs
; long_4-align_2-div_smoke.ll with the same alignment/remainder class and a
; smaller count; enable patmos-expensive-tests for the original large-count coverage.
; RUN: env MEMSET_COUNT=42 MEMSET_ALLOC_COUNT=42 MEMSET_PTR_INC=0 %memset_check_i32_main 0=0 1=42 2=84 6=252
; RUN: env MEMSET_COUNT=42 MEMSET_ALLOC_COUNT=42 MEMSET_PTR_INC=0 %memset_check_i32_indirect 0=0 1=42 2=84 6=252
; RUN: env MEMSET_COUNT=42 MEMSET_ALLOC_COUNT=42 MEMSET_PTR_INC=0 %memset_check_i64_main 0=0 1=42 2=84 6=252
; RUN: env MEMSET_COUNT=42 MEMSET_ALLOC_COUNT=42 MEMSET_PTR_INC=0 %memset_check_i64_indirect 0=0 1=42 2=84 6=252
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
;
; Tests can use llvm.memset without standard library 'memset' for:
; * High length
; * 4-Aligned start
; * 2-divisable length
;
;//////////////////////////////////////////////////////////////////////////////////////////////////
