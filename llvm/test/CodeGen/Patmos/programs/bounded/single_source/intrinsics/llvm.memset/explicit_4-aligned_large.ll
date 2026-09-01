; REQUIRES: patmos-expensive-tests
; This is the stress companion to explicit_4-aligned.ll. The default test uses
; the same explicit 4-byte alignment attributes with a smaller count; enable
; patmos-expensive-tests for this larger-count execution coverage.
; RUN: env MEMSET_COUNT=20 MEMSET_ALLOC_COUNT=20 MEMSET_PTR_INC=0 MEMSET_PTR_ATTR="noundef nonnull align 4" %memset_check_i32_main 0=0 1=20 2=40 6=120
; RUN: env MEMSET_COUNT=20 MEMSET_ALLOC_COUNT=20 MEMSET_PTR_INC=0 MEMSET_PTR_ATTR="noundef nonnull align 4" %memset_check_i32_indirect 0=0 1=20 2=40 6=120
; RUN: env MEMSET_COUNT=20 MEMSET_ALLOC_COUNT=20 MEMSET_PTR_INC=0 MEMSET_PTR_ATTR="noundef nonnull align 4" %memset_check_i64_main 0=0 1=20 2=40 6=120
; RUN: env MEMSET_COUNT=20 MEMSET_ALLOC_COUNT=20 MEMSET_PTR_INC=0 MEMSET_PTR_ATTR="noundef nonnull align 4" %memset_check_i64_indirect 0=0 1=20 2=40 6=120
