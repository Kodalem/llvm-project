; RUN: EXEC_ARGS="0=0 1=1 2=2 3=3 4=4 5=5"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
;
; Tests that calling a function that contains from a function that uses more than the available
; number of registers works.
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

@_1 = global i32 123
@_123 = global i32 123

define void @contains_loop(i32 %iteration_count){
entry:
  br label %loop
  
loop:
  %i.0 = phi i32 [ 0, %entry ], [ %inc, %loop ]
  %inc = add nsw i32 %i.0, 1
  %loaded = load volatile i32, i32* @_1
  store volatile i32 %loaded, i32* @_1
  %cmp = icmp slt i32 %i.0, %iteration_count
  call void @llvm.loop.bound(i32 0, i32 6)
  br i1 %cmp, label %loop, label %end
  
end:
  ret void
}
declare void @llvm.loop.bound(i32, i32)

; We use a naked function to have full control over register use.
; Therefore, we also need to remember to save srb/sro since we are making a call.
define i32 @main(i32 %x) {
entry:     
  %0 = load volatile i32, i32* @_1
  %1 = load volatile i32, i32* @_1
  %2 = load volatile i32, i32* @_1
  %3 = load volatile i32, i32* @_1
  %4 = load volatile i32, i32* @_1
  %5 = load volatile i32, i32* @_1
  %6 = load volatile i32, i32* @_1
  %7 = load volatile i32, i32* @_1
  %8 = load volatile i32, i32* @_1
  %9 = load volatile i32, i32* @_1
  %10 = load volatile i32, i32* @_1
  %11 = load volatile i32, i32* @_1
  %12 = load volatile i32, i32* @_1
  %13 = load volatile i32, i32* @_1
  %14 = load volatile i32, i32* @_1
  %15 = load volatile i32, i32* @_1
  %16 = load volatile i32, i32* @_1
  %17 = load volatile i32, i32* @_1
  %18 = load volatile i32, i32* @_1
  %19 = load volatile i32, i32* @_1
  %20 = load volatile i32, i32* @_1
  %21 = load volatile i32, i32* @_1
  %22 = load volatile i32, i32* @_1
  %23 = load volatile i32, i32* @_1
  %24 = load volatile i32, i32* @_1
  %25 = load volatile i32, i32* @_1
  %26 = load volatile i32, i32* @_1
  %27 = load volatile i32, i32* @_1
  %28 = load volatile i32, i32* @_1
  %29 = load volatile i32, i32* @_1
  %30 = load volatile i32, i32* @_1
  %31 = load volatile i32, i32* @_1
  call void @contains_loop(i32 %x)
  
  %_123 = load volatile i32, i32* @_123
  %corr0 = icmp eq i32 %0, %_123
  %corr1 = icmp eq i32 %1, %_123
  %corr2 = icmp eq i32 %2, %_123
  %corr3 = icmp eq i32 %3, %_123
  %corr4 = icmp eq i32 %4, %_123
  %corr5 = icmp eq i32 %5, %_123
  %corr6 = icmp eq i32 %6, %_123
  %corr7 = icmp eq i32 %7, %_123
  %corr8 = icmp eq i32 %8, %_123
  %corr9 = icmp eq i32 %9, %_123
  %corr10 = icmp eq i32 %10, %_123
  %corr11 = icmp eq i32 %11, %_123
  %corr12 = icmp eq i32 %12, %_123
  %corr13 = icmp eq i32 %13, %_123
  %corr14 = icmp eq i32 %14, %_123
  %corr15 = icmp eq i32 %15, %_123
  %corr16 = icmp eq i32 %16, %_123
  %corr17 = icmp eq i32 %17, %_123
  %corr18 = icmp eq i32 %18, %_123
  %corr19 = icmp eq i32 %19, %_123
  %corr20 = icmp eq i32 %20, %_123
  %corr21 = icmp eq i32 %21, %_123
  %corr22 = icmp eq i32 %22, %_123
  %corr23 = icmp eq i32 %23, %_123
  %corr24 = icmp eq i32 %24, %_123
  %corr25 = icmp eq i32 %25, %_123
  %corr26 = icmp eq i32 %26, %_123
  %corr27 = icmp eq i32 %27, %_123
  %corr28 = icmp eq i32 %28, %_123
  %corr29 = icmp eq i32 %29, %_123
  %corr30 = icmp eq i32 %30, %_123
  %corr31 = icmp eq i32 %31, %_123
  
  %result0 = select i1 %corr0, i32 %x, i32 100
  %result1 = select i1 %corr1, i32 %result0, i32 101
  %result2 = select i1 %corr2, i32 %result1, i32 102
  %result3 = select i1 %corr3, i32 %result2, i32 103
  %result4 = select i1 %corr4, i32 %result3, i32 104
  %result5 = select i1 %corr5, i32 %result4, i32 105
  %result6 = select i1 %corr6, i32 %result5, i32 106
  %result7 = select i1 %corr7, i32 %result6, i32 107
  %result8 = select i1 %corr8, i32 %result7, i32 108
  %result9 = select i1 %corr9, i32 %result8, i32 109
  %result10 = select i1 %corr10, i32 %result9, i32 110
  %result11 = select i1 %corr11, i32 %result10, i32 111
  %result12 = select i1 %corr12, i32 %result11, i32 112
  %result13 = select i1 %corr13, i32 %result12, i32 113
  %result14 = select i1 %corr14, i32 %result13, i32 114
  %result15 = select i1 %corr15, i32 %result14, i32 115
  %result16 = select i1 %corr16, i32 %result15, i32 116
  %result17 = select i1 %corr17, i32 %result16, i32 117
  %result18 = select i1 %corr18, i32 %result17, i32 118
  %result19 = select i1 %corr19, i32 %result18, i32 119
  %result20 = select i1 %corr20, i32 %result19, i32 120
  %result21 = select i1 %corr21, i32 %result20, i32 121
  %result22 = select i1 %corr22, i32 %result21, i32 122
  %result23 = select i1 %corr23, i32 %result22, i32 123
  %result24 = select i1 %corr24, i32 %result23, i32 124
  %result25 = select i1 %corr25, i32 %result24, i32 125
  %result26 = select i1 %corr26, i32 %result25, i32 126
  %result27 = select i1 %corr27, i32 %result26, i32 127
  %result28 = select i1 %corr28, i32 %result27, i32 128
  %result29 = select i1 %corr29, i32 %result28, i32 129
  %result30 = select i1 %corr30, i32 %result29, i32 130
  %result31 = select i1 %corr31, i32 %result30, i32 131
 
  store volatile i32 %result31, i32* @_123
    
  store volatile i32 %0, i32* @_1
  store volatile i32 %1, i32* @_1
  store volatile i32 %2, i32* @_1
  store volatile i32 %3, i32* @_1
  store volatile i32 %4, i32* @_1
  store volatile i32 %5, i32* @_1
  store volatile i32 %6, i32* @_1
  store volatile i32 %7, i32* @_1
  store volatile i32 %8, i32* @_1
  store volatile i32 %9, i32* @_1
  store volatile i32 %10, i32* @_1
  store volatile i32 %11, i32* @_1
  store volatile i32 %12, i32* @_1
  store volatile i32 %13, i32* @_1
  store volatile i32 %14, i32* @_1
  store volatile i32 %15, i32* @_1
  store volatile i32 %16, i32* @_1
  store volatile i32 %17, i32* @_1
  store volatile i32 %18, i32* @_1
  store volatile i32 %19, i32* @_1
  store volatile i32 %20, i32* @_1
  store volatile i32 %21, i32* @_1
  store volatile i32 %22, i32* @_1
  store volatile i32 %23, i32* @_1
  store volatile i32 %24, i32* @_1
  store volatile i32 %25, i32* @_1
  store volatile i32 %26, i32* @_1
  store volatile i32 %27, i32* @_1
  store volatile i32 %28, i32* @_1
  store volatile i32 %29, i32* @_1
  store volatile i32 %30, i32* @_1
  store volatile i32 %31, i32* @_1
  ret i32 %result31
}