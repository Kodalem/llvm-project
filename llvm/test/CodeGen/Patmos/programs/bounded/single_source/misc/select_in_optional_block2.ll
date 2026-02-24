; RUN: EXEC_ARGS="0=4 1=5 5=9 6=9 14=17 15=15"; \
; RUN: %test_execution
; END. 
;//////////////////////////////////////////////////////////////////////////////////////////////////
; 
; Tests a "select" instruction inside an optional block.
;
; This variation specifically tests that the select is used to enable or disable an arithmetic
; instruction (in this case 'add'). This is interesting because the select might be resolved
; by predicating the arithmetic instruction (e.g. instead of being turned into a conditional move).
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

@_1 = global i32 1
@_3 = global i32 3

define i32 @main(i32 %x) {
entry:
  %lt.15 = icmp ult i32 %x, 15
  %lt.6 = icmp ult i32 %x, 6
  br i1 %lt.15, label %if.then, label %end

if.then:                                      
  %_1 = load volatile i32, i32* @_1   
  %added = add i32 %x, %_1        
  %use.added= select i1 %lt.6, i32 %added, i32 %x
  %_3 = load volatile i32, i32* @_3
  %added2 = add i32 %use.added, %_3
  br label %end

end:        
  %result = phi i32 [%x, %entry], [%added2, %if.then]                 
  ret i32 %result
}
