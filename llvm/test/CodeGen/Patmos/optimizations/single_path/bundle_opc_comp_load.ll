; RUN: llc < %s --mpatmos-singlepath=main --mpatmos-enable-cet --mpatmos-disable-vliw=false \
; RUN:   --mpatmos-disable-singlepath-scheduler-equivalence-class=false \
; RUN:   --mpatmos-enable-permissive-dual-issue=true | FileCheck %s
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
;
; Tests that OPC compensation's compensatory loads may be bundled with the stores they compensate
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

; CHECK-LABEL: main:
define i32 @main(i32 %value, i32* %load_from)  {
entry:
  %is_odd = trunc i32 %value to i1
  br i1 %is_odd, label %if.true, label %end

if.true:
  store volatile i32 %value, i32* %load_from
  
; Check for a bundle with a compensation load and original store
; CHECK: { lwm (!$p1)
; CHECK-NEXT: swm ( $p1) 
  
  br label %end

end:
  ret i32 %value
}

