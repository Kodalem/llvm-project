; RUN: EXEC_ARGS="0=1 1=2"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
; 
; Tests calling a function that overwirtes the predicate registers ($s0) doesn't overwrite
; the callers predicates. (since s0 is a saved register per the register convension)
; 
;//////////////////////////////////////////////////////////////////////////////////////////////////

@_1 = global i32 1
@_2 = global i32 2

; If x is true, clears all predicate registers. If false, sets all predicates.
define void @change_s0_to(i1 %x)  {
entry:
  br i1 %x, label %if.one, label %if.zero
  
if.one:
  call void asm "
		mts $$s0 = $$r0		
	", "~{s0}"
	() ; All bits cleared
  br label %end

if.zero:
  call void asm "
		mts $$s0 = $0		
	", "r,~{s0}"
	(i32 -1) ; All bits set
  br label %end
  
end:
  ret void
}

define i32 @main(i32 %x)  {
entry:
  %is_zero = icmp eq i32 %x, 0
  call void @change_s0_to(i1 %is_zero)
  br i1 %is_zero, label %if.true, label %if.false
  
if.true:
  %0 = load volatile i32, i32* @_1
  br label %end
  
if.false:
  %1 = load volatile i32, i32* @_2
  br label %end
  
end:
  %result = phi i32 [ %0, %if.true ], [ %1, %if.false ]
  ret i32 %result
}
