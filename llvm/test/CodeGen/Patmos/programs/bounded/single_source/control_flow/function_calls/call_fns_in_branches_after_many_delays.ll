; RUN: EXEC_ARGS="0=0 1=36 2=64 3=100"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
; 
; Tests first having many instructions that use delay slots (here we just use many mults)
; followed by calls to functions from within branches.
;
; This test ensures that single-path code that manages function calls isn't reordered erroneously
; when unused delay slots appear before it.
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
  %two = load volatile i32, i32* @_2
  %mul1 = mul i32 %x, %two
  %mul2 = mul i32 %mul1, %two
  %mul3 = mul i32 %mul2, %two
  %mul4 = mul i32 %mul3, %two
  %mul5 = mul i32 %mul4, %two
  br i1 %is_odd, label %if.then, label %if.else
  
if.then:
  %plus1 = call i32 @add_1(i32 %mul5)
  %res.then = add i32 %plus1, 3
  br label %end

if.else:
  %plus2 = call i32 @add_2(i32 %mul5)
  %res.else = sub i32 %plus2, %two
  br label %end
  
end:
  %result = phi i32 [%res.then, %if.then], [%res.else, %if.else]
  ret i32 %result
}

