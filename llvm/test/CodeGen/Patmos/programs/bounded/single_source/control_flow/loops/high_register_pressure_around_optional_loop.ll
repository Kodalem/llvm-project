; RUN: EXEC_ARGS="0=58 1=58 2=58 3=64 4=68"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
; 
; Tests having many variable live past an optional loop.
;
; This ensures the optional loop doesn't affect the value of the variables (i.e. that 
; the values are spilled/loaded before/after the loop)
; 
;//////////////////////////////////////////////////////////////////////////////////////////////////

@_1 = global i32 1
@_2 = global i32 2
@for_checking = global i32 0

define i32 @main(i32 %iteration_count)  {
entry:
  ; We load volatile 29 value that will be used after the loop.
  %load.1 = load volatile i32, i32* @_2
  %load.2 = load volatile i32, i32* @_2
  %load.3 = load volatile i32, i32* @_2
  %load.4 = load volatile i32, i32* @_2
  %load.5 = load volatile i32, i32* @_2
  %load.6 = load volatile i32, i32* @_2
  %load.7 = load volatile i32, i32* @_2
  %load.8 = load volatile i32, i32* @_2
  %load.9 = load volatile i32, i32* @_2
  %load.10 = load volatile i32, i32* @_2
  %load.11 = load volatile i32, i32* @_2
  %load.12 = load volatile i32, i32* @_2
  %load.13 = load volatile i32, i32* @_2
  %load.14 = load volatile i32, i32* @_2
  %load.15 = load volatile i32, i32* @_2
  %load.16 = load volatile i32, i32* @_2
  %load.17 = load volatile i32, i32* @_2
  %load.18 = load volatile i32, i32* @_2
  %load.19 = load volatile i32, i32* @_2
  %load.20 = load volatile i32, i32* @_2
  %load.21 = load volatile i32, i32* @_2
  %load.22 = load volatile i32, i32* @_2
  %load.23 = load volatile i32, i32* @_2
  %load.24 = load volatile i32, i32* @_2
  %load.25 = load volatile i32, i32* @_2
  %load.26 = load volatile i32, i32* @_2
  %load.27 = load volatile i32, i32* @_2
  %load.28 = load volatile i32, i32* @_2
  %load.29 = load volatile i32, i32* @_2
  %lt.3 = icmp ult i32 %iteration_count, 3
  br i1 %lt.3, label %end, label %for.cond

for.cond:
  %x.0 = phi i32 [ 0, %entry ], [ %add1, %for.cond ]
  %i.0 = phi i32 [ 0, %entry ], [ %inc, %for.cond ]
  
  ; This load ensures the previous ones can't be moved to after the loop
  %0 = load volatile i32, i32* @_1
  
  %add = add nsw i32 %i.0, %0
  %add1 = add nsw i32 %x.0, %add
  %inc = add nsw i32 %i.0, 1
  %cmp = icmp slt i32 %i.0, %iteration_count
  call void @llvm.loop.bound(i32 0, i32 4)
  br i1 %cmp, label %for.cond, label %end

end:
  %x.end = phi i32 [0, %entry], [%x.0, %for.cond]
  
  ; We ensure each individual previous load stays live, we save and then reload it.
  ; We then add it to the sum to ensure it wasn't changed
  store volatile i32 %load.1, i32* @for_checking
  %load2.1 = load volatile i32, i32* @for_checking
  %x.1 = add i32 %load2.1, %x.end
  
  store volatile i32 %load.2, i32* @for_checking
  %load2.2 = load volatile i32, i32* @for_checking
  %x.2 = add i32 %load2.2, %x.1
  
  store volatile i32 %load.3, i32* @for_checking
  %load2.3 = load volatile i32, i32* @for_checking
  %x.3 = add i32 %load2.3, %x.2

  store volatile i32 %load.4, i32* @for_checking
  %load2.4 = load volatile i32, i32* @for_checking
  %x.4 = add i32 %load2.4, %x.3

  store volatile i32 %load.5, i32* @for_checking
  %load2.5 = load volatile i32, i32* @for_checking
  %x.5 = add i32 %load2.5, %x.4

  store volatile i32 %load.6, i32* @for_checking
  %load2.6 = load volatile i32, i32* @for_checking
  %x.6 = add i32 %load2.6, %x.5

  store volatile i32 %load.7, i32* @for_checking
  %load2.7 = load volatile i32, i32* @for_checking
  %x.7 = add i32 %load2.7, %x.6

  store volatile i32 %load.8, i32* @for_checking
  %load2.8 = load volatile i32, i32* @for_checking
  %x.8 = add i32 %load2.8, %x.7

  store volatile i32 %load.9, i32* @for_checking
  %load2.9 = load volatile i32, i32* @for_checking
  %x.9 = add i32 %load2.9, %x.8

  store volatile i32 %load.10, i32* @for_checking
  %load2.10 = load volatile i32, i32* @for_checking
  %x.10 = add i32 %load2.10, %x.9

  store volatile i32 %load.11, i32* @for_checking
  %load2.11 = load volatile i32, i32* @for_checking
  %x.11 = add i32 %load2.11, %x.10

  store volatile i32 %load.12, i32* @for_checking
  %load2.12 = load volatile i32, i32* @for_checking
  %x.12 = add i32 %load2.12, %x.11

  store volatile i32 %load.13, i32* @for_checking
  %load2.13 = load volatile i32, i32* @for_checking
  %x.13 = add i32 %load2.13, %x.12

  store volatile i32 %load.14, i32* @for_checking
  %load2.14 = load volatile i32, i32* @for_checking
  %x.14 = add i32 %load2.14, %x.13

  store volatile i32 %load.15, i32* @for_checking
  %load2.15 = load volatile i32, i32* @for_checking
  %x.15 = add i32 %load2.15, %x.14

  store volatile i32 %load.16, i32* @for_checking
  %load2.16 = load volatile i32, i32* @for_checking
  %x.16 = add i32 %load2.16, %x.15

  store volatile i32 %load.17, i32* @for_checking
  %load2.17 = load volatile i32, i32* @for_checking
  %x.17 = add i32 %load2.17, %x.16

  store volatile i32 %load.18, i32* @for_checking
  %load2.18 = load volatile i32, i32* @for_checking
  %x.18 = add i32 %load2.18, %x.17

  store volatile i32 %load.19, i32* @for_checking
  %load2.19 = load volatile i32, i32* @for_checking
  %x.19 = add i32 %load2.19, %x.18

  store volatile i32 %load.20, i32* @for_checking
  %load2.20 = load volatile i32, i32* @for_checking
  %x.20 = add i32 %load2.20, %x.19

  store volatile i32 %load.21, i32* @for_checking
  %load2.21 = load volatile i32, i32* @for_checking
  %x.21 = add i32 %load2.21, %x.20

  store volatile i32 %load.22, i32* @for_checking
  %load2.22 = load volatile i32, i32* @for_checking
  %x.22 = add i32 %load2.22, %x.21

  store volatile i32 %load.23, i32* @for_checking
  %load2.23 = load volatile i32, i32* @for_checking
  %x.23 = add i32 %load2.23, %x.22

  store volatile i32 %load.24, i32* @for_checking
  %load2.24 = load volatile i32, i32* @for_checking
  %x.24 = add i32 %load2.24, %x.23

  store volatile i32 %load.25, i32* @for_checking
  %load2.25 = load volatile i32, i32* @for_checking
  %x.25 = add i32 %load2.25, %x.24

  store volatile i32 %load.26, i32* @for_checking
  %load2.26 = load volatile i32, i32* @for_checking
  %x.26 = add i32 %load2.26, %x.25

  store volatile i32 %load.27, i32* @for_checking
  %load2.27 = load volatile i32, i32* @for_checking
  %x.27 = add i32 %load2.27, %x.26

  store volatile i32 %load.28, i32* @for_checking
  %load2.28 = load volatile i32, i32* @for_checking
  %x.28 = add i32 %load2.28, %x.27

  store volatile i32 %load.29, i32* @for_checking
  %load2.29 = load volatile i32, i32* @for_checking
  %x.29 = add i32 %load2.29, %x.28

  ret i32 %x.29
}

declare void @llvm.loop.bound(i32, i32)
