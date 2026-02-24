; RUN: EXEC_ARGS="0=180 1=181 2=182 3=183 4=184"; \
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
  %x.phi = phi i32 [ %x, %entry ], [ %x.phi2, %loop.inc ]
  %i = phi i32 [ 5, %entry ], [ %i.dec, %loop.inc ]
  %cmp = icmp eq i32 %i, 0
  call void @llvm.loop.bound(i32 5, i32 0)
  br i1 %cmp, label %end, label %loop2

loop.inc:
  %i.dec = sub nsw i32 %i, 1
  br label %loop
  
loop2:
  %x.phi2 = phi i32 [ %x.phi, %loop ], [ %x.phi3, %loop2.inc ]
  %j = phi i32 [ 3, %loop ], [ %j.dec, %loop2.inc ]
  %cmp2 = icmp eq i32 %j, 0
  call void @llvm.loop.bound(i32 3, i32 0)
  br i1 %cmp2, label %loop.inc, label %loop3

loop2.inc:
  %j.dec = sub nsw i32 %j, 1
  br label %loop2
  
loop3:
  %x.phi3 = phi i32 [ %x.phi2, %loop2 ], [ %x.add, %loop3.body ]
  %k = phi i32 [ 4, %loop2 ], [ %k.dec, %loop3.body ]
  %cmp3 = icmp eq i32 %k, 0
  call void @llvm.loop.bound(i32 4, i32 0)
  br i1 %cmp3, label %loop2.inc, label %loop3.body
  
loop3.body:
  %x.add = call i32 @add_three(i32 %x.phi3)
  %k.dec = sub nsw i32 %k, 1
  br label %loop3
  

end:
  ret i32 %x.phi
}

declare void @llvm.loop.bound(i32, i32)
