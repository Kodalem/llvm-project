; RUN: EXEC_ARGS="0=2 1=1 2=4 3=3"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
;
; Tests "triangle" branching (i.e. branch skips 1 block) with constant load amount (in this case
; we just put the loads at the beginning)
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

@_1 = global i32 1

define i32 @main(i32 %cond)  {
entry:
  %_1 = load volatile i32, i32* @_1
  %_12 = load volatile i32, i32* @_1
  %is.odd = trunc i32 %cond to i1
  br i1 %is.odd, label %if.end, label %if.then

if.then:
  %added = add i32 %_1, %_12
  %0 = add nsw i32 %cond, %added
  br label %if.end

if.end:
  %result = phi i32 [ %0, %if.then ], [ %cond, %entry]
  ret i32 %result
}
