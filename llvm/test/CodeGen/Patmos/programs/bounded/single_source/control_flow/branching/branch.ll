; RUN: EXEC_ARGS="0=3 1=3 2=4 3=5"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
;
; Tests simple symetric branching
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

@_1 = global i32 1

define i32 @main(i32 %cond)  {
entry:
  %tobool = icmp eq i32 %cond, 0
  br i1 %tobool, label %if.else, label %if.then

if.then:
  %_1 = load volatile i32, i32* @_1
  %_12 = load volatile i32, i32* @_1
  %added = add i32 %_1, %_12
  %0 = add nsw i32 %cond, %added
  br label %if.end

if.else:
  %1 = add nsw i32 %cond, 3
  br label %if.end

if.end:
  %result = phi i32 [ %0, %if.then ], [ %1, %if.else ]
  ret i32 %result
}
