; This is the default smoke version of short.ll. It checks representative
; one-byte and four-byte memset cases; run short.ll with expensive_checks
; enabled for every small count and input from the original exhaustive test.
; RUN: env MEMSET_COUNT=1 MEMSET_ALLOC_COUNT=10 MEMSET_PTR_INC=0 %memset_check_i32_main 0=0 2=2
; RUN: env MEMSET_COUNT=1 MEMSET_ALLOC_COUNT=10 MEMSET_PTR_INC=0 %memset_check_i32_indirect 0=0 2=2
; RUN: env MEMSET_COUNT=4 MEMSET_ALLOC_COUNT=10 MEMSET_PTR_INC=0 %memset_check_i64_main 0=0 2=8
; RUN: env MEMSET_COUNT=4 MEMSET_ALLOC_COUNT=10 MEMSET_PTR_INC=0 %memset_check_i64_indirect 0=0 2=8
