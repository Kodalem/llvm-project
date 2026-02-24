; RUN: EXEC_ARGS="0=0 1=0 8=1 9=2 10=3 17=4 18=6 19=8 27=12 45=30"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
; 
; Tests having a nested loop with blocks both before and after it in the outer loop.
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

@_1 = global i32 1

define i32 @main(i32 %x) {
entry:
  %0 = and i32 %x, 7
  %1 = and i32 %x, 56
  %2 = lshr i32 %1, 3
  br label %loop1

loop1:
  %loop1.i = phi i32 [0, %entry],[ %loop1.i.inc, %loop2]
  %result = phi i32 [0, %entry],[ %result.loop2.inc, %loop2]
  %_1.loop1 = load volatile i32, i32 * @_1
  %loop1.i.inc = add i32 %loop1.i, %_1.loop1
  %loop1.end = icmp uge i32 %loop1.i, %2
  call void @llvm.loop.bound(i32 0, i32 5)
  br i1 %loop1.end, label %end, label %loop2
  
loop2:
  %loop2.i = phi i32 [0, %loop1],[ %loop2.i.inc, %loop2]
  %result.loop2 = phi i32 [%result, %loop1],[ %result.loop2.inc, %loop2]
  %_1.loop2 = load volatile i32, i32 * @_1
  %loop2.i.inc = add i32 %loop2.i, %_1.loop2
  %result.loop2.inc = add i32 %result.loop2, %_1.loop2
  %loop2.loop = icmp ult i32 %loop2.i, %0
  call void @llvm.loop.bound(i32 0, i32 5)
  br i1 %loop2.loop, label %loop2, label %loop1
    
end:
  ret i32 %result
}

declare void @llvm.loop.bound(i32, i32)
