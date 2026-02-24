; RUN: EXEC_ARGS="0=4 9=13 10=12 14=16 15=13 17=15"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
;
; Tests can have a block with multiple predecessors that might be skipped altogether.
;
; This variation of the test doesn't use a PHI node in the optional block to ensure
; no additional blocks are added when the PHI is resolved.
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

@_4 = global i32 4
@_2 = global i32 2
@_x = global i32 0

define i32 @main(i32 %x)  {
entry:
  store volatile i32 %x, i32* @_x
  %lt.15 = icmp ult i32 %x, 15
  br i1 %lt.15, label %if.1, label %if.2

if.1:
  %_4 = load volatile i32, i32* @_4
  %added = add i32 %x, %_4
  store volatile i32 %added, i32* @_x
  %lt.10 = icmp ult i32 %x, 10
  br i1 %lt.10, label %end, label %if.2

if.2:
  %x.1 = load volatile i32, i32* @_x
  %_2 = load volatile i32, i32* @_2
  %subbed = sub i32 %x.1, %_2
  br label %end

end:
  %result = phi i32 [ %added, %if.1 ], [ %subbed, %if.2 ]
  ret i32 %result
}
