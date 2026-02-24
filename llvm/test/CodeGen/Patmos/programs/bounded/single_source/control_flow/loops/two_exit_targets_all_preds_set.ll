; RUN: EXEC_ARGS="0=100 1=101 15=107 11=5 20=5 24=106"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
; 
; Test that a loop with two exit targets, where the predicates are clobbered right before the
; the exit condition.
;
; This specifically tests that the single-path predicate used for exit target is initialized
; to false.
; 
;//////////////////////////////////////////////////////////////////////////////////////////////////
@_1 = global i32 1
@_100 = global i32 100
@_3 = global i32 3

define i32 @main(i32 %x) {
entry:
  br label %loop
loop:
  %x.1 = phi i32 [%x, %entry], [ %x.2, %loop_end]
  %exit_early = icmp ult i32 %x.1, 10
  call void @llvm.loop.bound(i32 0, i32 6)
  br i1 %exit_early, label %exit_target, label %loop_end
   
loop_end:
  %x.2 = lshr i32 %x.1, 1 ; divide by 2
  
  ; We set all predicates. This ensures any predicate used for the exit
  ; will have an initial value of true, unless its been initialized and maintained
  ; to false
  call void asm "
		pset $$p1
		pset $$p2
		pset $$p3
		pset $$p4
		pset $$p5
		pset $$p6
		pset $$p7
	", "~{s0}"
	()
	
  %exit_late = icmp ule i32 %x.2, 5
  br i1 %exit_late, label %end, label %loop
 
exit_target:
  %_100 = load volatile i32, i32* @_100
  %x.added = add i32 %x.1, %_100
  br label %end
  
end:
  %result = phi i32 [ %x.added, %exit_target ], [ %x.2, %loop_end ]
  ret i32 %result 
}

declare void @llvm.loop.bound(i32, i32)
