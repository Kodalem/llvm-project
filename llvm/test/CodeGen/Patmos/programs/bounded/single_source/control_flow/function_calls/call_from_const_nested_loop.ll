; RUN: EXEC_ARGS="0=165 1=166 2=167 3=168 4=169"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
; 
; Tests calling a function from a loop with constant bounds (i.e. min == max)
; 
;//////////////////////////////////////////////////////////////////////////////////////////////////

@_3 = global i32 3

define i32 @add_three(i32 %x)  {
entry:
  %0 = load volatile i32, i32* @_3
  %add = add i32 %x, %0
  ret i32 %add
}

define i32 @main(i32 %x)  {
entry:
  br label %loop
  
loop:
  %x.phi = phi i32 [ %x, %entry ], [ %x.add, %loop2 ]
  %i = phi i32 [ 5, %entry ], [ %i.dec, %loop2 ]
  %cmp = icmp eq i32 %i, 0
  %i.dec = sub nsw i32 %i, 1
  call void @llvm.loop.bound(i32 5, i32 0)
  br i1 %cmp, label %end, label %loop2
  
loop2:
  %x.phi2 = phi i32 [ %x.phi, %loop ], [ %x.add, %loop2 ]
  %j = phi i32 [ 10, %loop ], [ %j.dec, %loop2 ]
  %x.add = call i32 @add_three(i32 %x.phi2)
  %cmp2 = icmp eq i32 %j, 0
  %j.dec = sub nsw i32 %j, 1
  call void @llvm.loop.bound(i32 10, i32 0)
  br i1 %cmp2, label %loop, label %loop2

end:
  ret i32 %x.phi
}

declare void @llvm.loop.bound(i32, i32)
