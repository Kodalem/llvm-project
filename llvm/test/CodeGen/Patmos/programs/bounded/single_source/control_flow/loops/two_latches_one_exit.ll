; RUN: EXEC_ARGS="0=3 1=3 2=7 3=7 4=11"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
; 
; Test a loop with two latches and one exit (not in the header).
;
; This variation takes the second latch in the first loop iteration.
; 
;//////////////////////////////////////////////////////////////////////////////////////////////////
@_1 = global i32 1
@_2 = global i32 2

define i32 @main(i32 %x) {
entry:
  br label %loop
  
loop:
  %y = phi i32 [0, %entry], [ %y.one, %loop], [ %y.three, %loop.latch.exit]
  %i = phi i32 [0, %entry], [ %i.inc, %loop], [ %i.inc, %loop.latch.exit]
  %_1 = load volatile i32, i32* @_1
  %y.one = add i32 %y, %_1
  %i.inc = add i32 %i, %_1
  %is.odd = trunc i32 %y to i1
  call void @llvm.loop.bound(i32 0, i32 4)
  br i1 %is.odd, label %loop, label %loop.latch.exit
   
loop.latch.exit:
  %_2 = load volatile i32, i32* @_2
  %y.three = add i32 %y.one, %_2
  %exit = icmp uge i32 %i.inc, %x
  br i1 %exit, label %end, label %loop
  
end:
  ret i32 %y.three 
}

declare void @llvm.loop.bound(i32, i32)
