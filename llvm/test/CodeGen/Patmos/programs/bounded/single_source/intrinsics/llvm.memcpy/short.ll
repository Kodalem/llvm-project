; REQUIRES: patmos-expensive-tests
; This is the exhaustive small-count version. The default suite runs
; short_smoke.ll with representative small memcpy counts; enable
; patmos-expensive-tests for all counts and inputs.
;//////////////////////////////////////////////////////////////////////////////////////////////////
;
; Tests can use llvm.memcpy without needing the standard library "memcpy" for small values
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

; RUN: env MEMCPY_COUNT=1 MEMCPY_ALLOC_COUNT=10 MEMCPY_DEST_PTR_INC=0 MEMCPY_SRC_PTR_INC=0 %memcpy_check_i32_main 0=0 1=1 2=2 100=100
; RUN: env MEMCPY_COUNT=1 MEMCPY_ALLOC_COUNT=10 MEMCPY_DEST_PTR_INC=0 MEMCPY_SRC_PTR_INC=0 %memcpy_check_i32_indirect 0=0 1=1 2=2 100=100
; RUN: env MEMCPY_COUNT=1 MEMCPY_ALLOC_COUNT=10 MEMCPY_DEST_PTR_INC=0 MEMCPY_SRC_PTR_INC=0 %memcpy_check_i64_main 0=0 1=1 2=2 100=100
; RUN: env MEMCPY_COUNT=1 MEMCPY_ALLOC_COUNT=10 MEMCPY_DEST_PTR_INC=0 MEMCPY_SRC_PTR_INC=0 %memcpy_check_i64_indirect 0=0 1=1 2=2 100=100

; RUN: env MEMCPY_COUNT=2 MEMCPY_ALLOC_COUNT=10 MEMCPY_DEST_PTR_INC=0 MEMCPY_SRC_PTR_INC=0 %memcpy_check_i32_main 0=0 1=2 2=4 60=120
; RUN: env MEMCPY_COUNT=2 MEMCPY_ALLOC_COUNT=10 MEMCPY_DEST_PTR_INC=0 MEMCPY_SRC_PTR_INC=0 %memcpy_check_i32_indirect 0=0 1=2 2=4 60=120
; RUN: env MEMCPY_COUNT=2 MEMCPY_ALLOC_COUNT=10 MEMCPY_DEST_PTR_INC=0 MEMCPY_SRC_PTR_INC=0 %memcpy_check_i64_main 0=0 1=2 2=4 60=120
; RUN: env MEMCPY_COUNT=2 MEMCPY_ALLOC_COUNT=10 MEMCPY_DEST_PTR_INC=0 MEMCPY_SRC_PTR_INC=0 %memcpy_check_i64_indirect 0=0 1=2 2=4 60=120

; RUN: env MEMCPY_COUNT=3 MEMCPY_ALLOC_COUNT=10 MEMCPY_DEST_PTR_INC=0 MEMCPY_SRC_PTR_INC=0 %memcpy_check_i32_main 0=0 1=3 2=6 60=180
; RUN: env MEMCPY_COUNT=3 MEMCPY_ALLOC_COUNT=10 MEMCPY_DEST_PTR_INC=0 MEMCPY_SRC_PTR_INC=0 %memcpy_check_i32_indirect 0=0 1=3 2=6 60=180
; RUN: env MEMCPY_COUNT=3 MEMCPY_ALLOC_COUNT=10 MEMCPY_DEST_PTR_INC=0 MEMCPY_SRC_PTR_INC=0 %memcpy_check_i64_main 0=0 1=3 2=6 60=180
; RUN: env MEMCPY_COUNT=3 MEMCPY_ALLOC_COUNT=10 MEMCPY_DEST_PTR_INC=0 MEMCPY_SRC_PTR_INC=0 %memcpy_check_i64_indirect 0=0 1=3 2=6 60=180

; RUN: env MEMCPY_COUNT=4 MEMCPY_ALLOC_COUNT=10 MEMCPY_DEST_PTR_INC=0 MEMCPY_SRC_PTR_INC=0 %memcpy_check_i32_main 0=0 1=4 2=8 60=240
; RUN: env MEMCPY_COUNT=4 MEMCPY_ALLOC_COUNT=10 MEMCPY_DEST_PTR_INC=0 MEMCPY_SRC_PTR_INC=0 %memcpy_check_i32_indirect 0=0 1=4 2=8 60=240
; RUN: env MEMCPY_COUNT=4 MEMCPY_ALLOC_COUNT=10 MEMCPY_DEST_PTR_INC=0 MEMCPY_SRC_PTR_INC=0 %memcpy_check_i64_main 0=0 1=4 2=8 60=240
; RUN: env MEMCPY_COUNT=4 MEMCPY_ALLOC_COUNT=10 MEMCPY_DEST_PTR_INC=0 MEMCPY_SRC_PTR_INC=0 %memcpy_check_i64_indirect 0=0 1=4 2=8 60=240
