; RUN: llc < %s -mpatmos-enable-cet %XFAIL-filecheck %s
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
;
; Tests that if the 'mpatmos-enable-cet' is given, 
; but 'mpatmos-singlepath' is not, an error is thrown
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

define i32 @main() {
entry:
  ret i32 0
}

; CHECK: The 'mpatmos-enable-cet' option requires the 'mpatmos-singlepath' option to also be set.
