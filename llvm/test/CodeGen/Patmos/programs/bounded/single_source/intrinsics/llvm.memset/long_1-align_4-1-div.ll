; RUN: env MEMSET_COUNT=51 MEMSET_ALLOC_COUNT=52 MEMSET_PTR_INC=1 %memset_check_i32_main 0=0 1=51 2=102 5=255
; RUN: env MEMSET_COUNT=51 MEMSET_ALLOC_COUNT=52 MEMSET_PTR_INC=1 %memset_check_i32_indirect 0=0 1=51 2=102 5=255
; RUN: env MEMSET_COUNT=51 MEMSET_ALLOC_COUNT=52 MEMSET_PTR_INC=1 %memset_check_i64_main 0=0 1=51 2=102 5=255
; RUN: env MEMSET_COUNT=51 MEMSET_ALLOC_COUNT=52 MEMSET_PTR_INC=1 %memset_check_i64_indirect 0=0 1=51 2=102 5=255
; END
;//////////////////////////////////////////////////////////////////////////////////////////////////
;
; Tests can use llvm.memset without standard library 'memset' for:
; * High length
; * 1-Aligned start
; * 4-divisable length minus 1
;
;//////////////////////////////////////////////////////////////////////////////////////////////////
