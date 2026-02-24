; RUN: EXEC_ARGS="0=0 1=1 2=3 3=6 4=10"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
; 
; Tests a loop with a body that uses all general purpose registers, meaning some spilling/reloading
; will be needed around it.
; 
;//////////////////////////////////////////////////////////////////////////////////////////////////

@_1 = global i32 1

define i32 @main(i32 %iteration_count)  {
entry:
  br label %for.cond

for.cond:
  %x.0 = phi i32 [ 0, %entry ], [ %add1, %for.cond ]
  %i.0 = phi i32 [ 0, %entry ], [ %inc, %for.cond ]
  %0 = load volatile i32, i32* @_1
  %add = add nsw i32 %i.0, %0
  %add1 = add nsw i32 %x.0, %add
  %inc = add nsw i32 %i.0, 1
  
  ; We load volatile 30 values that we then store in reverse order
  ; to ensure they all need to be live together.
  %load.1 = load volatile i32, i32* @_1
  %load.2 = load volatile i32, i32* @_1
  %load.3 = load volatile i32, i32* @_1
  %load.4 = load volatile i32, i32* @_1
  %load.5 = load volatile i32, i32* @_1
  %load.6 = load volatile i32, i32* @_1
  %load.7 = load volatile i32, i32* @_1
  %load.8 = load volatile i32, i32* @_1
  %load.9 = load volatile i32, i32* @_1
  %load.10 = load volatile i32, i32* @_1
  %load.11 = load volatile i32, i32* @_1
  %load.12 = load volatile i32, i32* @_1
  %load.13 = load volatile i32, i32* @_1
  %load.14 = load volatile i32, i32* @_1
  %load.15 = load volatile i32, i32* @_1
  %load.16 = load volatile i32, i32* @_1
  %load.17 = load volatile i32, i32* @_1
  %load.18 = load volatile i32, i32* @_1
  %load.19 = load volatile i32, i32* @_1
  %load.20 = load volatile i32, i32* @_1
  %load.21 = load volatile i32, i32* @_1
  %load.22 = load volatile i32, i32* @_1
  %load.23 = load volatile i32, i32* @_1
  %load.24 = load volatile i32, i32* @_1
  %load.25 = load volatile i32, i32* @_1
  %load.26 = load volatile i32, i32* @_1
  %load.27 = load volatile i32, i32* @_1
  %load.28 = load volatile i32, i32* @_1
  %load.29 = load volatile i32, i32* @_1
  
  store volatile i32 %load.29, i32* @_1
  store volatile i32 %load.28, i32* @_1
  store volatile i32 %load.27, i32* @_1
  store volatile i32 %load.26, i32* @_1
  store volatile i32 %load.25, i32* @_1
  store volatile i32 %load.24, i32* @_1
  store volatile i32 %load.23, i32* @_1
  store volatile i32 %load.22, i32* @_1
  store volatile i32 %load.21, i32* @_1
  store volatile i32 %load.20, i32* @_1
  store volatile i32 %load.19, i32* @_1
  store volatile i32 %load.18, i32* @_1
  store volatile i32 %load.17, i32* @_1
  store volatile i32 %load.16, i32* @_1
  store volatile i32 %load.15, i32* @_1
  store volatile i32 %load.14, i32* @_1
  store volatile i32 %load.13, i32* @_1
  store volatile i32 %load.12, i32* @_1
  store volatile i32 %load.11, i32* @_1
  store volatile i32 %load.10, i32* @_1
  store volatile i32 %load.9, i32* @_1
  store volatile i32 %load.8, i32* @_1
  store volatile i32 %load.7, i32* @_1
  store volatile i32 %load.6, i32* @_1
  store volatile i32 %load.5, i32* @_1
  store volatile i32 %load.4, i32* @_1
  store volatile i32 %load.3, i32* @_1
  store volatile i32 %load.2, i32* @_1
  store volatile i32 %load.1, i32* @_1
  
  %cmp = icmp slt i32 %i.0, %iteration_count
  call void @llvm.loop.bound(i32 0, i32 4)
  br i1 %cmp, label %for.cond, label %for.end

for.end:
  ret i32 %x.0
}

declare void @llvm.loop.bound(i32, i32)
