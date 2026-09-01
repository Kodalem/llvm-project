;//////////////////////////////////////////////////////////////////////////////////////////////////
;
; Tests can use llvm.memset without needing the standard library "memset" for large values
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

; RUN: env MEMSET_COUNT=513 MEMSET_ALLOC_COUNT=513 MEMSET_PTR_INC=0 %memset_check_i32_main 0=0 1=1 2=2 6=6
; RUN: env MEMSET_COUNT=513 MEMSET_ALLOC_COUNT=513 MEMSET_PTR_INC=0 %memset_check_i32_indirect 0=0 1=1 2=2 6=6
; RUN: env MEMSET_COUNT=513 MEMSET_ALLOC_COUNT=513 MEMSET_PTR_INC=0 %memset_check_i64_main 0=0 1=1 2=2 6=6
; RUN: env MEMSET_COUNT=513 MEMSET_ALLOC_COUNT=513 MEMSET_PTR_INC=0 %memset_check_i64_indirect 0=0 1=1 2=2 6=6
