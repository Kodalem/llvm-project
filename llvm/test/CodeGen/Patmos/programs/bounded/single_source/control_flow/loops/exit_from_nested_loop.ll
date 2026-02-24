; RUN: EXEC_ARGS="0=0 1=0 8=0 9=1 10=1 12=1 16=0 17=2 18=4 19=2 27=9 32=0 33=4 34=8 35=12 45=25"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
; 
; Tests having an edge that exits a loop from its inner loop
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

define i32 @main(i32 %x) {
entry:
  %loop2.limit = and i32 %x, 7 ; Extracts the first 3 bits (0,1,2) to be used as number
  %0 = and i32 %x, 56 ; Extracts bits 3,4,5
  %both.loop.limit = lshr i32 %0, 3 ; Shift right so bits 3,4,5 can be treated as a number
  br label %loop1

loop1:
  %loop1.i = phi i32 [0, %entry],[ %loop1.i.inc, %loop2]
  %result = phi i32 [0, %entry],[ %result.loop2, %loop2]
  %loop1.i.inc = add i32 %loop1.i, 1
  %loop1.end = icmp uge i32 %loop1.i, %both.loop.limit
  call void @llvm.loop.bound(i32 0, i32 5)
  br i1 %loop1.end, label %end, label %loop2
  
loop2:
  %loop2.i = phi i32 [0, %loop1],[ %loop2.i.inc, %loop2.post]
  %result.loop2 = phi i32 [%result, %loop1],[ %result.loop2.inc, %loop2.post]
  %loop2.i.inc = add i32 %loop2.i, 1
  %result.loop2.inc = add i32 %result.loop2, 1
  %loop2.loop = icmp ult i32 %loop2.i, %loop2.limit
  call void @llvm.loop.bound(i32 0, i32 5)
  br i1 %loop2.loop, label %loop2.post, label %loop1
  
loop2.post:
  %loop2.break = icmp uge i32 %loop2.i, %both.loop.limit
  br i1 %loop2.break, label %end, label %loop2

end:
  %result.end = phi i32 [%result, %loop1], [%result.loop2, %loop2.post]
  ret i32 %result.end
}

declare void @llvm.loop.bound(i32, i32)
