; RUN: EXEC_ARGS="0=1 1=5 2=3 3=7 4=5 5=9"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
; 
; Tests calling different functions from different paths
; 
;//////////////////////////////////////////////////////////////////////////////////////////////////

@_1 = global i32 1
@_2 = global i32 2

define i32 @add_1(i32 %x)  {
entry:
  %0 = load volatile i32, i32* @_1
  %result = add i32 %x, %0
  ret i32 %result
}

define i32 @add_2(i32 %x)  {
entry:
  %0 = load volatile i32, i32* @_2
  %result = add i32 %x, %0
  ret i32 %result
}

define i32 @main(i32 %x)  {
entry:
  %is_odd = trunc i32 %x to i1
  br i1 %is_odd, label %if.then, label %if.else
  
if.then:
  %plus1 = call i32 @add_1(i32 %x)
  %res.then = add i32 %plus1, 3
  br label %end

if.else:
  %plus2 = call i32 @add_2(i32 %x)
  %res.else = sub i32 %plus2, 1
  br label %end
  
end:
  %result = phi i32 [%res.then, %if.then], [%res.else, %if.else]
  ret i32 %result
}

