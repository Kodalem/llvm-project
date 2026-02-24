; RUN: EXEC_ARGS="0=3 1=4 5=8 6=7 14=15 15=15"; \
; RUN: %test_execution
; END. 
;//////////////////////////////////////////////////////////////////////////////////////////////////
; 
; Tests a "select" instruction inside an optional block
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
  %_3 = load volatile i32, i32* @_3
  %use.loaded = select i1 %lt.6, i32 %_3, i32 %_1
  %added = add i32 %x, %use.loaded
  br label %end

end:        
  %result = phi i32 [%x, %entry], [%added, %if.then]                 
  ret i32 %result
}
