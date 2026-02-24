; RUN: EXEC_ARGS="0=3 1=3 2=6 3=9 5=15 16=48"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
; 
; Tests that a loop with the condition at the end (do-while) works.
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

@_1 = global i32 0

define i32 @main(i32 %iteration_count)  {
entry:
  br label %while.body

 
while.body:
  %x = phi i32 [ 0, %entry ], [ %addx, %while.cond ]
  %addx = add i32 %x, 2
  call void @llvm.loop.bound(i32 0, i32 15)
  br label %while.cond
  
while.cond:
  %0 = load volatile i32, i32* @_1
  %inc = add i32 %0, 1
  store volatile i32 %inc, i32* @_1
  %cmp = icmp slt i32 %inc, %iteration_count
  br i1 %cmp, label %while.body, label %end

end:
  %1 = load volatile i32, i32* @_1
  %result = add i32 %addx, %1
  ret i32 %result
}

declare void @llvm.loop.bound(i32, i32)
