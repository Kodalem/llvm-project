; RUN: EXEC_ARGS="0=5 1=6 2=7 3=8 4=9"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
; 
; Tests calling a function from a loop with constant bounds (i.e. min == max)
; where the call is in a block that might not be executed on the last loop iteration
; 
;//////////////////////////////////////////////////////////////////////////////////////////////////

@_1 = global i32 1

define i32 @add_one(i32 %x)  {
entry:
  %0 = load volatile i32, i32* @_1
  %add = add i32 %x, %0
  ret i32 %add
}

define i32 @main(i32 %x)  {
entry:
  br label %loop
  
loop:
  %x.phi = phi i32 [ %x, %entry ], [ %x.add, %loop.body ]
  %i = phi i32 [ 5, %entry ], [ %i.dec, %loop.body ]
  %cmp = icmp eq i32 %i, 0
  call void @llvm.loop.bound(i32 5, i32 0)
  br i1 %cmp, label %end, label %loop.body
  
loop.body:
  %i.dec = sub nsw i32 %i, 1
  %x.add = call i32 @add_one(i32 %x.phi)
  br label %loop

end:
  ret i32 %x.phi
}

declare void @llvm.loop.bound(i32, i32)
