; This is the default smoke version of long_1-align_2-div.ll. It preserves the
; 1-byte pointer offset and 2-divisible length class with a smaller count. Run
; long_1-align_2-div.ll with expensive_checks enabled for large-count coverage.
; RUN: env MEMSET_COUNT=6 MEMSET_ALLOC_COUNT=7 MEMSET_PTR_INC=1 %memset_check_i32_main 0=0 2=12
; RUN: env MEMSET_COUNT=6 MEMSET_ALLOC_COUNT=7 MEMSET_PTR_INC=1 %memset_check_i32_indirect 0=0 2=12
; RUN: env MEMSET_COUNT=6 MEMSET_ALLOC_COUNT=7 MEMSET_PTR_INC=1 %memset_check_i64_main 0=0 2=12
; RUN: env MEMSET_COUNT=6 MEMSET_ALLOC_COUNT=7 MEMSET_PTR_INC=1 %memset_check_i64_indirect 0=0 2=12
