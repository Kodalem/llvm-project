; RUN: env MEMSET_COUNT=43 MEMSET_ALLOC_COUNT=43 MEMSET_PTR_INC=0 %memset_check_i32_main 0=0 1=43 2=86 5=215
; RUN: env MEMSET_COUNT=43 MEMSET_ALLOC_COUNT=43 MEMSET_PTR_INC=0 %memset_check_i32_indirect 0=0 1=43 2=86 5=215
; RUN: env MEMSET_COUNT=43 MEMSET_ALLOC_COUNT=43 MEMSET_PTR_INC=0 %memset_check_i64_main 0=0 1=43 2=86 5=215
; RUN: env MEMSET_COUNT=43 MEMSET_ALLOC_COUNT=43 MEMSET_PTR_INC=0 %memset_check_i64_indirect 0=0 1=43 2=86 5=215
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
;
; Tests can use llvm.memset without standard library 'memset' for:
; * High length
; * 4-Aligned start
; * 4-divisable length minus 1
;
;//////////////////////////////////////////////////////////////////////////////////////////////////
