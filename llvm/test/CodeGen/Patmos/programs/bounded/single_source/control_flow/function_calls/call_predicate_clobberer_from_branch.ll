; RUN: EXEC_ARGS="0=6 1=2 2=2 3=2"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
; 
; Tests call calling a function that overwirtes the predicate registers ($s0) from a branch path.
; 
;//////////////////////////////////////////////////////////////////////////////////////////////////

@_1 = global i32 1
@_2 = global i32 2

; If x is true, clears all predicate registers. If false, sets all predicates.
define i32 @change_s0_to(i1 %x)  {
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
  %_1 = load volatile i32, i32*@_1
  %_4 = add i32 %_1, 3
  ret i32 %_4
}

define i32 @main(i32 %x)  {
entry:
  %is_zero = icmp eq i32 %x, 0
  br i1 %is_zero, label %if.true, label %if.false
  
if.true:
  %0 = load volatile i32, i32* @_1
  %_4 = call i32 @change_s0_to(i1 %is_zero)
  %added = add i32 %0, %_4
  br label %end
  
if.false:
  %1 = load volatile i32, i32* @_2
  br label %end
  
end:
  %result = phi i32 [ %added, %if.true ], [ %1, %if.false ]
  %is_zero_i32 = zext i1 %is_zero to i32
  %result.added = add i32 %result, %is_zero_i32
  ret i32 %result.added
}
