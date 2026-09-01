; RUN: %test_execution_main 1=1 2=0 3=1 11=1 100=0
; RUN: %test_execution_indirect 1=1 2=0 3=1 11=1 100=0
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
;
; Tests a situation where an integer is truncated into an i1, compared with something,
; and the result used as a branch condition.
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

define i32 @main(i32 %x) {
entry:
  %t = trunc i32 %x to i1
  %tobool = icmp ne i1 %t, 0
  br i1 %tobool, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  br label %if.end

if.else:                                          ; preds = %entry
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  %res = phi i32 [ 1, %if.then ], [ 0, %if.else ]
  ret i32 %res
}
