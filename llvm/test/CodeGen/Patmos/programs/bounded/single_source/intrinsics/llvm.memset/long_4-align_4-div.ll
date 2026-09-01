; RUN: env MEMSET_COUNT=40 MEMSET_ALLOC_COUNT=40 MEMSET_PTR_INC=0 %memset_check_i32_main 0=0 1=40 2=80 6=240
; RUN: env MEMSET_COUNT=40 MEMSET_ALLOC_COUNT=40 MEMSET_PTR_INC=0 %memset_check_i32_indirect 0=0 1=40 2=80 6=240
; RUN: env MEMSET_COUNT=40 MEMSET_ALLOC_COUNT=40 MEMSET_PTR_INC=0 %memset_check_i64_main 0=0 1=40 2=80 6=240
; RUN: env MEMSET_COUNT=40 MEMSET_ALLOC_COUNT=40 MEMSET_PTR_INC=0 %memset_check_i64_indirect 0=0 1=40 2=80 6=240
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
;
; Tests can use llvm.memset without standard library 'memset' for:
; * High length
; * 4-Aligned start
; * 4-divisable length
;
;//////////////////////////////////////////////////////////////////////////////////////////////////
