; This is the default smoke version of short.ll. It checks representative
; one-byte and four-byte memcpy cases; run short.ll with expensive_checks
; enabled for every small count and input from the original exhaustive test.
; RUN: env MEMCPY_COUNT=1 MEMCPY_ALLOC_COUNT=10 MEMCPY_DEST_PTR_INC=0 MEMCPY_SRC_PTR_INC=0 %memcpy_check_i32_main 0=0 2=2
; RUN: env MEMCPY_COUNT=1 MEMCPY_ALLOC_COUNT=10 MEMCPY_DEST_PTR_INC=0 MEMCPY_SRC_PTR_INC=0 %memcpy_check_i32_indirect 0=0 2=2
; RUN: env MEMCPY_COUNT=4 MEMCPY_ALLOC_COUNT=10 MEMCPY_DEST_PTR_INC=0 MEMCPY_SRC_PTR_INC=0 %memcpy_check_i64_main 0=0 2=8
; RUN: env MEMCPY_COUNT=4 MEMCPY_ALLOC_COUNT=10 MEMCPY_DEST_PTR_INC=0 MEMCPY_SRC_PTR_INC=0 %memcpy_check_i64_indirect 0=0 2=8
