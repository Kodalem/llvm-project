; RUN: EXEC_ARGS="0=0 1=14 2=28 3=42 4=56 5=70"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
; 
; Tests having a nested loop with blocks both before and after it in the outer loop.
;
; This variation tests a constant outer loop with a variable inner loop
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

@_1 = global i32 1
@_3 = global i32 3

define i32 @main(i32 %x) {
entry:
  %_3 = load volatile i32, i32 * @_3
  br label %loop1

loop1:
  %loop1.i = phi i32 [0, %entry],[ %loop1.i.inc, %loop1.post]
  %result = phi i32 [0, %entry],[ %result.shift, %loop1.post]
  %_1.loop1 = load volatile i32, i32 * @_1
  %loop1.i.inc = add i32 %loop1.i, %_1.loop1
  %loop1.end = icmp uge i32 %loop1.i, %_3
  call void @llvm.loop.bound(i32 3, i32 0)
  br i1 %loop1.end, label %end, label %loop2
  
loop2:
  %loop2.i = phi i32 [0, %loop1],[ %loop2.i.inc, %loop2]
  %result.loop2 = phi i32 [%result, %loop1],[ %result.loop2.inc, %loop2]
  %_1.loop2 = load volatile i32, i32 * @_1
  %loop2.i.inc = add i32 %loop2.i, %_1.loop2
  %result.loop2.inc = add i32 %result.loop2, %_1.loop2
  %loop2.loop = icmp ult i32 %loop2.i, %x
  call void @llvm.loop.bound(i32 0, i32 5)
  br i1 %loop2.loop, label %loop2, label %loop1.post
  
loop1.post:
  %_1.loop1.post = load volatile i32, i32 * @_1
  %result.shift = shl i32 %result.loop2, %_1.loop1.post
  br label %loop1
  
end:
  ret i32 %result
}

declare void @llvm.loop.bound(i32, i32)
