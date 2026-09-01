;      # We reuse the main method by compiling it manually
; RUN: llc %S/matches_method_cache_size.ll -filetype=obj -o %t_match -mpatmos-max-subfunction-size=64
;      # Compile this test and startup code, then link all objects.
; RUN: llc %s -filetype=obj -o %t_prog && llc %S/../../_start.ll -filetype=obj -o %t_start && ld.lld --nostdlib --static -o %t_exec %t_start %t_prog %t_match
;      # Execute with a smaller method cache; this should fail.
; RUN: pasim %t_exec -c 800 --mcsize=32 %XFAIL-filecheck %s
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
;
; Tests that setting the subfunction size to be higher than the method cache size will cause an
; execution error, as the function blocks will be too large for the method cache.
;
; This test is here to indirectly tests that setting the max subfunction size will result in
; blocks of that size. We check the size of the blocks by running the program on the simulator
; with a smaller cache than the block size, meaning it should fail if the blocks are the correct
; size.
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

; CHECK: Method cache size exceeded