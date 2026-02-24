; RUN: EXEC_ARGS="0=5 1=10 2=15 3=20 4=25"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
; 
; Tests a constant loop containing a variable loop
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

@_1 = global i32 1
@_5 = global i32 5

define i32 @main(i32 %x) {
entry:
  br label %loop1

loop1:
  %loop1.i = phi i32 [0, %entry],[ %loop1.i.inc, %loop2]
  %result = phi i32 [0, %entry],[ %result.loop2.inc, %loop2]
  %_5 = load volatile i32, i32 * @_5
  %loop1.i.inc = add i32 %loop1.i, 1
  %i.lt.5 = icmp ult i32 %loop1.i, %_5
  call void @llvm.loop.bound(i32 5, i32 0)
  br i1 %i.lt.5, label %loop2, label %end
  
loop2:
  %loop2.i = phi i32 [0, %loop1],[ %loop2.i.inc, %loop2]
  %result.loop2 = phi i32 [%result, %loop1],[ %result.loop2.inc, %loop2]
  %_1 = load volatile i32, i32 * @_1
  %loop2.i.inc = add i32 %loop2.i, %_1
  %result.loop2.inc = add i32 %result.loop2, %_1
  %loop2.loop = icmp ult i32 %loop2.i, %x
  call void @llvm.loop.bound(i32 0, i32 4)
  br i1 %loop2.loop, label %loop2, label %loop1
    
end:
  ret i32 %result
}

declare void @llvm.loop.bound(i32, i32)
