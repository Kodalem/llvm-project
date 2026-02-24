; RUN: EXEC_ARGS="0=0 1=7 2=14 4=0 5=10 16=0 21=15 22=30 30=42 62=62"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
; 
; Tests having two consecutive loops inside another loop.
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

@_1 = global i32 1
@_3 = global i32 3
@_5 = global i32 5
@result = global i32 0

define i32 @main(i32 %x) {
entry:
  %loop.1.count = and i32 %x, 3
  %0 = and i32 %x, 12
  %loop.2.count = lshr i32 %0, 2
  %1 = and i32 %x, 48
  %loop.3.count = lshr i32 %1, 4
  br label %loop.1

loop.1:
  %loop.1.i = phi i32 [0, %entry], [%loop.1.i.inc, %loop.1.end]
  %loop.1.continue = icmp ult i32 %loop.1.i, %loop.1.count
  call void @llvm.loop.bound(i32 0, i32 4)
  br i1 %loop.1.continue, label %loop.1.1, label %end
  
loop.1.1:
  %_1.1 = load volatile i32, i32* @_1
  %result.1 = load volatile i32, i32* @result
  %result.1.2 = add i32 %result.1, %_1.1
  store volatile i32 %result.1.2, i32* @result
  br label %loop.2

loop.2:
  %loop.2.i = phi i32 [0, %loop.1.1], [%loop.2.i.inc, %loop.2.body]
  %loop.2.continue = icmp ult i32 %loop.2.i, %loop.2.count
  call void @llvm.loop.bound(i32 0, i32 4)
  br i1 %loop.2.continue, label %loop.2.body, label %loop.1.2

loop.2.body:
  %_3 = load volatile i32, i32* @_3
  %result.2 = load volatile i32, i32* @result
  %result.2.2 = add i32 %result.2, %_3
  store volatile i32 %result.2.2, i32* @result
  %loop.2.i.inc = add i32 %loop.2.i, 1
  br label %loop.2

loop.1.2:
  %result.3 = load volatile i32, i32* @result
  %result.3.2 = add i32 %result.3, %_1.1
  store volatile i32 %result.3.2, i32* @result
  br label %loop.3

loop.3:
  %loop.3.i = phi i32 [0, %loop.1.2], [%loop.3.i.inc, %loop.3.body]
  %loop.3.continue = icmp ult i32 %loop.3.i, %loop.3.count
  %_5 = load volatile i32, i32* @_5
  %result.4 = load volatile i32, i32* @result
  %result.4.2 = add i32 %result.4, %_5
  store volatile i32 %result.4.2, i32* @result
  call void @llvm.loop.bound(i32 0, i32 4)
  br i1 %loop.3.continue, label %loop.3.body, label %loop.1.end

loop.3.body:
  %loop.3.i.inc = add i32 %loop.3.i, 1
  br label %loop.3

loop.1.end:
  %loop.1.i.inc = add i32 %loop.1.i, 1
  br label %loop.1
    
end:
  %result.end = load volatile i32, i32* @result
  ret i32 %result.end
}

declare void @llvm.loop.bound(i32, i32)
