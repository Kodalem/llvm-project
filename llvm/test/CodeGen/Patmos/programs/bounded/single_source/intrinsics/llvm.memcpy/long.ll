; REQUIRES: patmos-expensive-tests
; This is the stress version. The default suite runs long_smoke.ll with the
; same memcpy lowering path and a smaller copy length; enable expensive_checks
; for the original large-count coverage.
;//////////////////////////////////////////////////////////////////////////////////////////////////
;
; Tests can use llvm.memcpy without needing the standard library "memcpy" for large values
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

; RUN: env MEMCPY_COUNT=513 MEMCPY_ALLOC_COUNT=513 MEMCPY_DEST_PTR_INC=0 MEMCPY_SRC_PTR_INC=0 %memcpy_check_i32_main 0=0 1=1 2=2 6=6
; RUN: env MEMCPY_COUNT=513 MEMCPY_ALLOC_COUNT=513 MEMCPY_DEST_PTR_INC=0 MEMCPY_SRC_PTR_INC=0 %memcpy_check_i32_indirect 0=0 1=1 2=2 6=6
; RUN: env MEMCPY_COUNT=513 MEMCPY_ALLOC_COUNT=513 MEMCPY_DEST_PTR_INC=0 MEMCPY_SRC_PTR_INC=0 %memcpy_check_i64_main 0=0 1=1 2=2 6=6
; RUN: env MEMCPY_COUNT=513 MEMCPY_ALLOC_COUNT=513 MEMCPY_DEST_PTR_INC=0 MEMCPY_SRC_PTR_INC=0 %memcpy_check_i64_indirect 0=0 1=1 2=2 6=6
