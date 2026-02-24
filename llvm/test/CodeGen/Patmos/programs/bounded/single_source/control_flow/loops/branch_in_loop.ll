; RUN: EXEC_ARGS="0=1 1=4 2=3 3=8 4=5"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
; 
; 
; 
;//////////////////////////////////////////////////////////////////////////////////////////////////

@_1 = global i32 1
@_2 = global i32 2

define i32 @main(i32 %iteration_count)  {
entry:
  %is_odd = trunc i32 %iteration_count to i1
  br label %for.start

for.start:                                         ; preds = %for.inc, %entry
  %x.0 = phi i32 [ 0, %entry ], [ %x.3, %for.end ]
  %i.0 = phi i32 [ 0, %entry ], [ %inc, %for.end ]
  call void @llvm.loop.bound(i32 0, i32 5)
  br i1 %is_odd, label %if.do, label %if.else

if.do:
  %_2 = load volatile i32, i32*@_2
  %x.1 = add i32 %x.0, %_2
  br label %for.end
  
if.else:
  %_1 = load volatile i32, i32*@_1
  %x.2 = add i32 %x.0, %_1
  br label %for.end


for.end:  
  %x.3 = phi i32 [%x.1, %if.do], [%x.2, %if.else]
  %inc = add nsw i32 %i.0, 1
  %cmp = icmp slt i32 %i.0, %iteration_count
  br i1 %cmp, label %for.start, label %end

end:                                          
  ret i32 %x.3
}

declare void @llvm.loop.bound(i32, i32)
