; REQUIRES: patmos-expensive-tests
; This is the exhaustive small-count version. The default suite runs
; short_smoke.ll with representative small memset counts; enable
; patmos-expensive-tests for all counts and inputs.
;//////////////////////////////////////////////////////////////////////////////////////////////////
;
; Tests can use llvm.memset without needing the standard library "memset" for small values
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

; RUN: env MEMSET_COUNT=1 MEMSET_ALLOC_COUNT=10 MEMSET_PTR_INC=0 %memset_check_i32_main 0=0 1=1 2=2 100=100
; RUN: env MEMSET_COUNT=1 MEMSET_ALLOC_COUNT=10 MEMSET_PTR_INC=0 %memset_check_i32_indirect 0=0 1=1 2=2 100=100
; RUN: env MEMSET_COUNT=1 MEMSET_ALLOC_COUNT=10 MEMSET_PTR_INC=0 %memset_check_i64_main 0=0 1=1 2=2 100=100
; RUN: env MEMSET_COUNT=1 MEMSET_ALLOC_COUNT=10 MEMSET_PTR_INC=0 %memset_check_i64_indirect 0=0 1=1 2=2 100=100

; RUN: env MEMSET_COUNT=2 MEMSET_ALLOC_COUNT=10 MEMSET_PTR_INC=0 %memset_check_i32_main 0=0 1=2 2=4 60=120
; RUN: env MEMSET_COUNT=2 MEMSET_ALLOC_COUNT=10 MEMSET_PTR_INC=0 %memset_check_i32_indirect 0=0 1=2 2=4 60=120
; RUN: env MEMSET_COUNT=2 MEMSET_ALLOC_COUNT=10 MEMSET_PTR_INC=0 %memset_check_i64_main 0=0 1=2 2=4 60=120
; RUN: env MEMSET_COUNT=2 MEMSET_ALLOC_COUNT=10 MEMSET_PTR_INC=0 %memset_check_i64_indirect 0=0 1=2 2=4 60=120

; RUN: env MEMSET_COUNT=3 MEMSET_ALLOC_COUNT=10 MEMSET_PTR_INC=0 %memset_check_i32_main 0=0 1=3 2=6 60=180
; RUN: env MEMSET_COUNT=3 MEMSET_ALLOC_COUNT=10 MEMSET_PTR_INC=0 %memset_check_i32_indirect 0=0 1=3 2=6 60=180
; RUN: env MEMSET_COUNT=3 MEMSET_ALLOC_COUNT=10 MEMSET_PTR_INC=0 %memset_check_i64_main 0=0 1=3 2=6 60=180
; RUN: env MEMSET_COUNT=3 MEMSET_ALLOC_COUNT=10 MEMSET_PTR_INC=0 %memset_check_i64_indirect 0=0 1=3 2=6 60=180

; RUN: env MEMSET_COUNT=4 MEMSET_ALLOC_COUNT=10 MEMSET_PTR_INC=0 %memset_check_i32_main 0=0 1=4 2=8 60=240
; RUN: env MEMSET_COUNT=4 MEMSET_ALLOC_COUNT=10 MEMSET_PTR_INC=0 %memset_check_i32_indirect 0=0 1=4 2=8 60=240
; RUN: env MEMSET_COUNT=4 MEMSET_ALLOC_COUNT=10 MEMSET_PTR_INC=0 %memset_check_i64_main 0=0 1=4 2=8 60=240
; RUN: env MEMSET_COUNT=4 MEMSET_ALLOC_COUNT=10 MEMSET_PTR_INC=0 %memset_check_i64_indirect 0=0 1=4 2=8 60=240
