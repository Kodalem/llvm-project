; This is the default smoke version of long_4-align_1-div.ll. It preserves the
; 4-byte-aligned start and 1-divisible length class with a smaller count. Run
; long_4-align_1-div.ll with expensive_checks enabled for large-count coverage.
; RUN: env MEMSET_COUNT=5 MEMSET_ALLOC_COUNT=5 MEMSET_PTR_INC=0 %memset_check_i32_main 0=0 2=10
; RUN: env MEMSET_COUNT=5 MEMSET_ALLOC_COUNT=5 MEMSET_PTR_INC=0 %memset_check_i32_indirect 0=0 2=10
; RUN: env MEMSET_COUNT=5 MEMSET_ALLOC_COUNT=5 MEMSET_PTR_INC=0 %memset_check_i64_main 0=0 2=10
; RUN: env MEMSET_COUNT=5 MEMSET_ALLOC_COUNT=5 MEMSET_PTR_INC=0 %memset_check_i64_indirect 0=0 2=10
