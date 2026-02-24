; Function that can compensate for the variability in main memory accesses.
;
; "max" is the maximum number of extra enabled accesses a function can need
; "remaining" is the number of enabled accesses a given call to the function
; is missing.
; 
; __patmos_main_mem_access_compensation performs "remaining" number of enabled
; loads to main memory.
;
; __patmos_main_mem_access_compensation has a custom calling convention:
; it can only clobber r23(max),r24(remaining), p1, and p2.
@__patmos_main_mem_access_compensation = alias void (), void ()* @__patmos_comp_fun_for_testing
@__patmos_main_mem_access_compensation_di = alias void (), void ()* @__patmos_comp_fun_for_testing

define void @__patmos_comp_fun_for_testing() #0 {  
entry:
  call void asm sideeffect "
		__patmos_comp_fun_for_testing_loop:
		cmpult			$$p1 = $$r0, $$r23
		cmpult			$$p2 = $$r0, $$r24
		lwm		($$p2)	$$r0 = [$$r0]
		br 		($$p1) 	__patmos_comp_fun_for_testing_loop
		sub 	($$p2) 	$$r24 = $$r24, 1
		sub 			$$r23 = $$r23, 1
		retnd
	", ""
	()
  unreachable
}
attributes #0 = { naked noinline }

