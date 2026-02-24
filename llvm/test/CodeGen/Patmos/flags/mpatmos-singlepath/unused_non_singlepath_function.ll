; RUN: llc %s -mpatmos-singlepath="main" -o %t
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
;
; Tests that we can have a function in the 'llvm.used' list, that isn't used by singlepath code,
; and isn't convertable to singlepath.
; 
;//////////////////////////////////////////////////////////////////////////////////////////////////

define i32 @main(i32 %x) {
entry:
  %add = add i32 %x, 2
  ret i32 %add
}

@llvm.used = appending global [1 x i8*] [i8* bitcast (i32 (i32, i32)* @unused_fn to i8*)], section "llvm.metadata"

define i32 @unused_fn(i32 %x, i32 %x2) #1 {
entry:
  %add = add i32 %x, %x2
  br label %loop
  
loop:
  ; Inbounded loop isn't allowed in singlepath
  %add2 = add i32 %x, %x2
  br label %loop
}