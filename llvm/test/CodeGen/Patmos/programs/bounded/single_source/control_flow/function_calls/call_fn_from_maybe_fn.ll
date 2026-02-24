; RUN: EXEC_ARGS="0=5 1=1 2=7 3=3 4=9"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
; 
; Tests that a function that is only sometimes called can call another function (always)
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

@_2 = global i32 2

define i32 @always_called(i32 %x)  {
entry:
  %add = add nsw i32 %x, 3
  ret i32 %add
}

define i32 @maybe_called(i32 %x)  {
entry:
  %0 = load volatile i32, i32* @_2
  %add = add nsw i32 %0, %x
  %1 = call i32 @always_called(i32 %add)
  ret i32 %1
}

define i32 @main(i32 %x)  {
entry:
  %first_bit = and i32 %x, 1
  %is_even = icmp eq i32 %first_bit, 0
  br i1 %is_even, label %cond.false, label %cond.true

cond.true:                                        ; preds = %entry
  br label %cond.end

cond.false:                                       ; preds = %entry
  %call = call i32 @maybe_called(i32 %x)
  br label %cond.end

cond.end:                                         ; preds = %cond.false, %cond.true
  %cond = phi i32 [ %x, %cond.true ], [ %call, %cond.false ]
  ret i32 %cond
}
