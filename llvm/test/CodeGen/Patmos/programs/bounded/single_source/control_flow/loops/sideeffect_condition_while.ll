; RUN: EXEC_ARGS="0=1 1=4 2=7 3=10 5=16 16=49"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
; 
; Tests that a loop with the condition a the beginning.
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

@_1 = global i32 0

define i32 @main(i32 %iteration_count)  {
entry:
  br label %while.cond

while.cond:
  %x = phi i32 [ 0, %entry ], [ %addx, %while.body ]
  %0 = load volatile i32, i32* @_1
  %inc = add i32 %0, 1
  store volatile i32 %inc, i32* @_1
  %cmp = icmp slt i32 %0, %iteration_count
  call void @llvm.loop.bound(i32 0, i32 16)
  br i1 %cmp, label %while.body, label %end
 
while.body:
  %addx = add i32 %x, 2
  br label %while.cond

end:
  %1 = load volatile i32, i32* @_1
  %result = add i32 %x, %1
  ret i32 %result
}

declare void @llvm.loop.bound(i32, i32)
