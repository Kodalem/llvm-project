; RUN: llc < %s --mpatmos-singlepath=main --mpatmos-enable-cet=opposite --mpatmos-disable-vliw=false \
; RUN: | FileCheck %s
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
;
; Tests that OPC compensation's compensatory loads may be bundled with instructions from other,
; equivalence-class-dependent instructions (in this case adds).
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

; CHECK-LABEL: main:
define i32 @main(i32 %value, i32* %load_from)  {
entry:
  %is_odd = trunc i32 %value to i1
  %_1 = load volatile i32, i32* %load_from
  br i1 %is_odd, label %block.1, label %end

block.1:
  %is_odd2 = trunc i32 %_1 to i1
  store volatile i32 %value, i32* %load_from
  store volatile i32 %value, i32* %load_from
  store volatile i32 %value, i32* %load_from
  store volatile i32 %value, i32* %load_from
  store volatile i32 %value, i32* %load_from
  store volatile i32 %value, i32* %load_from
  store volatile i32 %value, i32* %load_from
  store volatile i32 %value, i32* %load_from
  store volatile i32 %value, i32* %load_from
  store volatile i32 %value, i32* %load_from
  store volatile i32 %value, i32* %load_from
  store volatile i32 %value, i32* %load_from
  store volatile i32 %value, i32* %load_from
  br i1 %is_odd2, label %block.2, label %end
  
block.2:
  ; We use %_1 as input to ensure these adds have no scheduling dependency on the above stores.
  %add.1 = add i32 %_1, %_1
  %add.2 = add i32 %add.1, %_1
  %add.3 = add i32 %add.2, %_1
  %add.4 = add i32 %add.3, %_1
  %add.5 = add i32 %add.4, %_1
  %add.6 = add i32 %add.5, %_1
  %add.7 = add i32 %add.6, %_1
  %add.8 = add i32 %add.7, %_1
  %add.9 = add i32 %add.8, %_1
  %add.10 = add i32 %add.9, %_1
  %add.11 = add i32 %add.10, %_1
  %add.12 = add i32 %add.11, %_1
  %add.13 = add i32 %add.12, %_1
  br label %end

; Check for a bundling between block.1's compensatory loads and block.2's adds
; CHECK: { lwm (!$p{{[0-9]}}) $r0 = [0]{{[[:space:]].*}}add ( $p
;  

end:
  %result = phi i32 [%_1, %entry], [%_1, %block.1], [%add.13, %block.2]
  ret i32 %result
}

