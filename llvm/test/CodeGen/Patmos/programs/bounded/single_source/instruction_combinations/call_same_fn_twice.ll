; RUN: EXEC_ARGS="0=2 1=3 2=4 3=5 4=6 5=7"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
;
; Tests can call a function twice from the same function
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

define i32 @main(i32 %x) {
entry:     
  %0 = call i32 @add_2(i32 %x)
  %1 = call i32 @add_2(i32 %0)
  ret i32 %0
}

define i32 @add_2(i32 %x) {
entry:  
  %0 = add i32 %x, 2 
  ret i32 %0
}
