; This is the default smoke version of long.ll. It keeps the same generated
; memcpy template and aligned source/destination shape, but uses a smaller
; copy length. Run long.ll with expensive_checks enabled for the original
; 513-byte stress coverage.
; RUN: env MEMCPY_COUNT=16 MEMCPY_ALLOC_COUNT=16 MEMCPY_DEST_PTR_INC=0 MEMCPY_SRC_PTR_INC=0 %memcpy_check_i32_main 0=0 2=32
; RUN: env MEMCPY_COUNT=16 MEMCPY_ALLOC_COUNT=16 MEMCPY_DEST_PTR_INC=0 MEMCPY_SRC_PTR_INC=0 %memcpy_check_i32_indirect 0=0 2=32
; RUN: env MEMCPY_COUNT=16 MEMCPY_ALLOC_COUNT=16 MEMCPY_DEST_PTR_INC=0 MEMCPY_SRC_PTR_INC=0 %memcpy_check_i64_main 0=0 2=32
; RUN: env MEMCPY_COUNT=16 MEMCPY_ALLOC_COUNT=16 MEMCPY_DEST_PTR_INC=0 MEMCPY_SRC_PTR_INC=0 %memcpy_check_i64_indirect 0=0 2=32
