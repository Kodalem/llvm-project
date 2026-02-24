; RUN: llc %s -mpatmos-singlepath=main -o %t %XFAIL-filecheck %s
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
; 
; Tests that having a mutually recursive functions within single-path context results in an error
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

@_1 = global i32 1

define i32 @main(i32 %x)  {
entry:
  %0 = call i32 @mutually_recursive(i32 %x)
  ret i32 %0
}

define i32 @mutually_recursive(i32 %x)  {
entry:
  %0 = call i32 @main(i32 %x)
  ret i32 %0
}
; CHECK: Single-path code generation failed due to recursive call to 'mutually_recursive' in 'main'!
