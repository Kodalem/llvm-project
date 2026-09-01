; RUN: env MEMSET_COUNT=52 MEMSET_ALLOC_COUNT=53 MEMSET_PTR_INC=1 %memset_check_i32_main 0=0 1=52 2=104 4=208
; RUN: env MEMSET_COUNT=52 MEMSET_ALLOC_COUNT=53 MEMSET_PTR_INC=1 %memset_check_i32_indirect 0=0 1=52 2=104 4=208
; RUN: env MEMSET_COUNT=52 MEMSET_ALLOC_COUNT=53 MEMSET_PTR_INC=1 %memset_check_i64_main 0=0 1=52 2=104 4=208
; RUN: env MEMSET_COUNT=52 MEMSET_ALLOC_COUNT=53 MEMSET_PTR_INC=1 %memset_check_i64_indirect 0=0 1=52 2=104 4=208
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
;
; Tests can use llvm.memset without standard library 'memset' for:
; * High length
; * 1-Aligned start
; * 4-divisable length
;
;//////////////////////////////////////////////////////////////////////////////////////////////////
