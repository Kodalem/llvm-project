; RUN: EXEC_ARGS="0=5 1=6 2=7 4=9 100=105"; \
; RUN: %test_execution
; END. 
;//////////////////////////////////////////////////////////////////////////////////////////////////
;
; Tests can call an aliased function
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

@alias_fn = alias i32 (i32), i32 (i32)* @helper_fn

define i32 @helper_fn(i32 %x) {
  %add = add i32 %x, 5
  ret i32 %add
}

define i32 @main(i32 %x)  {
  %result = call i32 @alias_fn(i32 %x)
  ret i32 %result
}
