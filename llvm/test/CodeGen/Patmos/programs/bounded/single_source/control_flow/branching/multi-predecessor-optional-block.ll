; RUN: EXEC_ARGS="0=4 9=13 10=12 14=16 15=13 17=15"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
;
; Tests can have a block with multiple predecessors that might be skipped altogether.
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

@_4 = global i32 4
@_2 = global i32 2

define i32 @main(i32 %x)  {
entry:
  %lt.15 = icmp ult i32 %x, 15
  br i1 %lt.15, label %if.1, label %if.2

if.1:
  %_4 = load volatile i32, i32* @_4
  %added = add i32 %x, %_4
  %lt.10 = icmp ult i32 %x, 10
  br i1 %lt.10, label %end, label %if.2

if.2:
  %x.1 = phi i32 [%x, %entry], [%added, %if.1]
  %_2 = load volatile i32, i32* @_2
  %subbed = sub i32 %x.1, %_2
  br label %end

end:
  %result = phi i32 [ %added, %if.1 ], [ %subbed, %if.2 ]
  ret i32 %result
}
