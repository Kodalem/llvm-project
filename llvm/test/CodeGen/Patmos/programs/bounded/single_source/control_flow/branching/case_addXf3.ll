; RUN: EXEC_ARGS="0=1 1=2 2=5 3=13 4=18 5=15"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
;
; Tests a specific case inpired by the '__addXf3__.1' newlib standard library function.
;
; This excercises the single-path equivalence class scheduling to notice that
; blocks 4 and 2 are equivalence class dependent and therefore can't be reordered.
; If they are reordered, block 4's save will happen after block 2's loads, meaning
; the '4' value is not added to the result.
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

@_4 = global i32 0

define i32 @main(i32 %cond)  {
entry:
  %lt3 = icmp ult i32 %cond, 3
  br i1 %lt3, label %block.1, label %block.3

block.3:
  %eq4 = icmp eq i32 %cond, 4
  %add.3 = add i32 %cond, 3
  br i1 %eq4, label %block.4, label %block.5

block.4:
  store volatile i32 4, i32* @_4
  br label %block.5

block.5:
  %add.5 = add i32 %add.3, 5
  br label %block.2

block.1:
  %eq2 = icmp eq i32 %cond, 2
  %add.1 = add i32 %cond, 1
  br i1 %eq2, label %block.2, label %end
  
block.2:
  %phi.2 = phi i32 [%add.1, %block.1], [%add.5, %block.5]
  %_4 = load volatile i32, i32* @_4
  %add.21 = add i32 %phi.2, %_4
  %add.2 = add i32 %add.21, 2
  br label %end
  
end:
  %result = phi i32 [ %add.1, %block.1 ], [ %add.2, %block.2 ]
  ret i32 %result
}
