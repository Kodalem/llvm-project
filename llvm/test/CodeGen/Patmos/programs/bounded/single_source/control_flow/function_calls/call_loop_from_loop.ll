; RUN: EXEC_ARGS="0=1 1=3 2=6 3=10 4=15"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
; 
; Tests calling a function containing a loop from another function's loop
; 
;//////////////////////////////////////////////////////////////////////////////////////////////////

@_1 = global i32 1

define i32 @add_to(i32 %x, i32 %iterations)  {
entry:
  br label %loop
  
loop:
  %x.phi = phi i32 [ %x, %entry ], [ %x.add, %loop ]
  %i = phi i32 [ %iterations, %entry ], [ %i.dec, %loop ]
  %cmp = icmp eq i32 %i, 0
  %i.dec = sub nsw i32 %i, 1
  %one = load volatile i32, i32* @_1
  %x.add = add i32 %x.phi, %one
  call void @llvm.loop.bound(i32 0, i32 4)
  br i1 %cmp, label %end, label %loop

end:
  ret i32 %x.add
}

define i32 @main(i32 %x)  {
entry:
  br label %loop
  
loop:
  %x.phi = phi i32 [ 0, %entry ], [ %x.add, %loop ]
  %i = phi i32 [ %x, %entry ], [ %i.dec, %loop ]
  %cmp = icmp eq i32 %i, 0
  %i.dec = sub nsw i32 %i, 1
  %x.add = call i32 @add_to(i32 %x.phi, i32 %i)
  call void @llvm.loop.bound(i32 0, i32 4)
  br i1 %cmp, label %end, label %loop

end:
  ret i32 %x.add
}

declare void @llvm.loop.bound(i32, i32)
