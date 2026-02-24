; RUN: EXEC_ARGS="0=1 1=2"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
;
; Tests that using a phi node for predicates works.
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

@_1 = global i32 1

define i32 @main(i32 %cond)  {
entry:
  %is_odd = trunc i32 %cond to i1
  br i1 %is_odd, label %if.then, label %if.end

if.then:
  %_1 = load volatile i32, i32* @_1
  %trunked = trunc i32 %_1 to i1
  ; This will always be true. We use it to ensure %is_odd and %trunked can't be
  ; assigned the same physical register (ensuring a pmove is needed for the phi)
  %anded = and i1 %trunked, %is_odd
  %anded32 = zext i1 %anded to i32
  store volatile i32 %anded32, i32* @_1
  br label %if.end

if.end:
  %pred = phi i1 [ %is_odd, %entry ], [ %trunked, %if.then]
  %pred32 = zext i1 %pred to i32
  %result = add i32 %pred32, 1
  ret i32 %result
}


