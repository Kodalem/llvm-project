; RUN: EXEC_ARGS="0=87 1=88 2=89 3=93 4=97 5=102"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
; 
; Tests that a high-register-pressure function can optionally call a function 
; containing a loop with high register pressure.
; 
;//////////////////////////////////////////////////////////////////////////////////////////////////

@_1 = global i32 1
@_3 = global i32 3

define i32 @main(i32 %x) {
entry:
  ; We load volatile 30 values that we then use after the (potential) function call
  %load.1 = load volatile i32, i32* @_3
  %load.2 = load volatile i32, i32* @_3
  %load.3 = load volatile i32, i32* @_3
  %load.4 = load volatile i32, i32* @_3
  %load.5 = load volatile i32, i32* @_3
  %load.6 = load volatile i32, i32* @_3
  %load.7 = load volatile i32, i32* @_3
  %load.8 = load volatile i32, i32* @_3
  %load.9 = load volatile i32, i32* @_3
  %load.10 = load volatile i32, i32* @_3
  %load.11 = load volatile i32, i32* @_3
  %load.12 = load volatile i32, i32* @_3
  %load.13 = load volatile i32, i32* @_3
  %load.14 = load volatile i32, i32* @_3
  %load.15 = load volatile i32, i32* @_3
  %load.16 = load volatile i32, i32* @_3
  %load.17 = load volatile i32, i32* @_3
  %load.18 = load volatile i32, i32* @_3
  %load.19 = load volatile i32, i32* @_3
  %load.20 = load volatile i32, i32* @_3
  %load.21 = load volatile i32, i32* @_3
  %load.22 = load volatile i32, i32* @_3
  %load.23 = load volatile i32, i32* @_3
  %load.24 = load volatile i32, i32* @_3
  %load.25 = load volatile i32, i32* @_3
  %load.26 = load volatile i32, i32* @_3
  %load.27 = load volatile i32, i32* @_3
  %load.28 = load volatile i32, i32* @_3
  %load.29 = load volatile i32, i32* @_3
  
  %lt.3 = icmp ult i32 %x, 3
  br i1 %lt.3, label %end, label %if.true
  
if.true:
  %0 = call i32 @pressure(i32 %x)
  br label %end
  
end:
  %result = phi i32 [%x, %entry], [%0, %if.true]
  
  %add.1 = add i32 %load.29, %result
  %add.2 = add i32 %load.28, %add.1
  %add.3 = add i32 %load.27, %add.2
  %add.4 = add i32 %load.26, %add.3
  %add.5 = add i32 %load.25, %add.4
  %add.6 = add i32 %load.24, %add.5
  %add.7 = add i32 %load.23, %add.6
  %add.8 = add i32 %load.22, %add.7
  %add.9 = add i32 %load.21, %add.8
  %add.10 = add i32 %load.20, %add.9
  %add.11 = add i32 %load.19, %add.10
  %add.12 = add i32 %load.18, %add.11
  %add.13 = add i32 %load.17, %add.12
  %add.14 = add i32 %load.16, %add.13
  %add.15 = add i32 %load.15, %add.14
  %add.16 = add i32 %load.14, %add.15
  %add.17 = add i32 %load.13, %add.16
  %add.18 = add i32 %load.12, %add.17
  %add.19 = add i32 %load.11, %add.18
  %add.20 = add i32 %load.10, %add.19
  %add.21 = add i32 %load.9, %add.20
  %add.22 = add i32 %load.8, %add.21
  %add.23 = add i32 %load.7, %add.22
  %add.24 = add i32 %load.6, %add.23
  %add.25 = add i32 %load.5, %add.24
  %add.26 = add i32 %load.4, %add.25
  %add.27 = add i32 %load.3, %add.26
  %add.28 = add i32 %load.2, %add.27
  %add.29 = add i32 %load.1, %add.28
  
  ret i32 %add.29
}

define i32 @pressure(i32 %iteration_count)  {
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
  call void @llvm.loop.bound(i32 0, i32 5)
  br i1 %cmp, label %for.cond, label %for.end

for.end:
  ret i32 %x.0
}

declare void @llvm.loop.bound(i32, i32)
