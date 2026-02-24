; RUN: EXEC_ARGS="0=0 1=1 2=20 3=3 4=4 5=5 6=60"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
; 
; Tests having a loop with a latch and a nested loop that exits both.
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

@_1 = global i32 1
@_3 = global i32 3
@result = global i32 0

define i32 @main(i32 %x) {
entry:
  br label %loop1.header

loop1.header:
  %i.1 = phi i32 [0, %entry], [%i.1.inc, %loop1.latch], [%i.1.inc, %loop2.header]
  %x.1 = phi i32 [0, %entry], [%x.1.added, %loop1.latch], [%x.2, %loop2.header]
  %i.1.inc = add i32 %i.1, 1
  %_1 = load volatile i32, i32* @_1
  %x.1.added = add i32 %x.1, %_1
  %lt.x = icmp ult i32 %i.1, %x
  call void @llvm.loop.bound(i32 0, i32 10)
  br i1 %lt.x, label %loop1.latch, label %end
  
loop1.latch:
  %is.odd = trunc i32 %x to i1
  br i1 %is.odd, label %loop1.header, label %loop2.header

loop2.header:
  %i.2 = phi i32 [0, %loop1.latch], [%i.2.inc, %loop2.exit]
  %x.2 = phi i32 [%x.1.added, %loop1.latch], [%x.2.added, %loop2.exit]
  %i.2.inc = add i32 %i.2, 1
  %_3 = load volatile i32, i32* @_3
  %x.2.added = add i32 %x.2, %_3
  %lt.3 = icmp ult i32 %i.2, %_3
  call void @llvm.loop.bound(i32 3, i32 0)
  br i1 %lt.3, label %loop2.exit, label %loop1.header
  
loop2.exit:
  %x.eq.4 = icmp eq i32 %x, 4
  br i1 %x.eq.4, label %end, label %loop2.header

end:
  %result = phi i32 [%x.1, %loop1.header], [%x.2.added, %loop2.exit]
  ret i32 %result
}

declare void @llvm.loop.bound(i32, i32)
