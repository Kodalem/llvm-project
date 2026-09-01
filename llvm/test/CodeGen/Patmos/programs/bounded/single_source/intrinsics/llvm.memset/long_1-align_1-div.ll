; RUN: env MEMSET_COUNT=49 MEMSET_ALLOC_COUNT=50 MEMSET_PTR_INC=1 %memset_check_i32_main 0=0 1=49 2=98 5=245
; RUN: env MEMSET_COUNT=49 MEMSET_ALLOC_COUNT=50 MEMSET_PTR_INC=1 %memset_check_i32_indirect 0=0 1=49 2=98 5=245
; RUN: env MEMSET_COUNT=49 MEMSET_ALLOC_COUNT=50 MEMSET_PTR_INC=1 %memset_check_i64_main 0=0 1=49 2=98 5=245
; RUN: env MEMSET_COUNT=49 MEMSET_ALLOC_COUNT=50 MEMSET_PTR_INC=1 %memset_check_i64_indirect 0=0 1=49 2=98 5=245
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
;
; Tests can use llvm.memset without standard library 'memset' for:
; * High length
; * 1-Aligned start
; * 1-divisable length
;
;//////////////////////////////////////////////////////////////////////////////////////////////////
