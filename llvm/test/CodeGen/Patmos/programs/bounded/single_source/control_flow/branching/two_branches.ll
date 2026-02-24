; RUN: EXEC_ARGS="0=4 1=4 2=7 3=6"; \
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
  %is_odd = trunc i32 %result to i1
  br i1 %is_odd, label %if2.then, label %if2.else
  
if2.then:
  %_13 = load volatile i32, i32* @_1
  %added2 = add i32 %_13, %result
  br label %end

if2.else:
  %2 = add nsw i32 %result, 3
  br label %end
  
end:
  %result2 = phi i32 [ %added2, %if2.then ], [ %2, %if2.else ]
  ret i32 %result2
}
