; RUN: EXEC_ARGS="0=2 1=4"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
; 
; Tests calling a function between calculating a branch condition and using that condition.
;
; This ensures the called function will not alter the predicate register values and thus
; corrupt the condition before the branch
; 
;//////////////////////////////////////////////////////////////////////////////////////////////////

@_1 = global i32 1
@_2 = global i32 2

define i32 @add_one(i32 %x)  {
entry:
  %0 = load volatile i32, i32* @_1
  %add = add i32 %x, %0
  ret i32 %add
}

define i32 @main(i32 %x)  {
entry:
  %is_zero = icmp eq i32 %x, 0
  %x_inc = call i32 @add_one(i32 %x)
  br i1 %is_zero, label %if.true, label %if.false
  
if.true:
  %0 = load volatile i32, i32* @_1
  br label %end
  
if.false:
  %1 = load volatile i32, i32* @_2
  br label %end
  
end:
  %result.0 = phi i32 [ %0, %if.true ], [ %1, %if.false ]
  %result = add i32 %x_inc, %result.0
  ret i32 %result
}
