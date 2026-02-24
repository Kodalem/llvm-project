; RUN: EXEC_ARGS="0=1 1=3 2=5 3=7 5=11 16=33"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
; 
; Tests that a loop with the condition not in the header, nor at the end of the body.
;
; We use a condition that includes branches, to ensure that the condition block
; cannot be simplified and merged into the header.
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

@_1 = global i32 0
@_2 = global i32 0

define i32 @main(i32 %iteration_count)  {
entry:
  br label %while.head

while.head:
  %x = phi i32 [ 0, %entry ], [ %addx, %while.body ]
  %trunc_x = trunc i32 %x to i1
  call void @llvm.loop.bound(i32 0, i32 16)
  br i1 %trunc_x, label %while.cond.true, label %while.cond.false
  
while.cond.true:
  %load1 = load volatile i32, i32* @_1
  br label %while.cond
  
while.cond.false:
  %load2 = load volatile i32, i32* @_2
  br label %while.cond

while.cond:
  %loaded = phi i32 [ %load1, %while.cond.true ], [ %load2, %while.cond.false ]
  %store_to = phi i32* [ @_1, %while.cond.true ], [ @_2, %while.cond.false ]
  %load_other = phi i32* [ @_2, %while.cond.true ], [ @_1, %while.cond.false ]
  %inc = add i32 %loaded, 1
  store volatile i32 %inc, i32* %store_to
  %other = load volatile i32, i32* %load_other
  %load_added = add i32 %loaded, %other
  %cmp = icmp slt i32 %load_added, %iteration_count
  br i1 %cmp, label %while.body, label %end

while.body:
  %addx = add i32 %x, 1
  br label %while.head

end:
  %0 = load volatile i32, i32* @_1
  %1 = load volatile i32, i32* @_2
  %result = add i32 %x, %0
  %result2 = add i32 %result, %1
  ret i32 %result2
}

declare void @llvm.loop.bound(i32, i32)
