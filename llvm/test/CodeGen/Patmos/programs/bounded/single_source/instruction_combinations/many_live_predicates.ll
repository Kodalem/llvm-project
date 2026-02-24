; RUN: EXEC_ARGS="0=0 1=1 7=1 8=1 9=0 10=0"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
; 
; Tests deep if/else nesting. Because the if/else nesting depth is 7, spilling at least 1 register is required.
; 
;//////////////////////////////////////////////////////////////////////////////////////////////////

@cond = global i32 0
@cond2 = global i1 0

define i32 @init_func()  {
entry:  
  ; We create 8 i1 values such that there are more predicates
  ; to manage than the availabel predicate registers.
  
  ; We store after each compare to ensure that comparison
  ; cannot be moved to later (and such needing less live predicates 
  ; by comparing right before they are needed)

  %ld1 = load volatile i32, i32* @cond
  %is1 = icmp eq i32 %ld1, 1
  store volatile i1 %is1, i1* @cond2
  %ld2 = load volatile i32, i32* @cond
  %is2 = icmp eq i32 %ld2, 2
  store volatile i1 %is2, i1* @cond2
  %ld3 = load volatile i32, i32* @cond
  %is3 = icmp eq i32 %ld3, 3
  store volatile i1 %is3, i1* @cond2
  %ld4 = load volatile i32, i32* @cond
  %is4 = icmp eq i32 %ld4, 4
  store volatile i1 %is4, i1* @cond2
  %ld5 = load volatile i32, i32* @cond
  %is5 = icmp eq i32 %ld5, 5
  store volatile i1 %is5, i1* @cond2
  %ld6 = load volatile i32, i32* @cond
  %is6 = icmp eq i32 %ld6, 6
  store volatile i1 %is6, i1* @cond2
  %ld7 = load volatile i32, i32* @cond
  %is7 = icmp eq i32 %ld7, 7
  store volatile i1 %is7, i1* @cond2
  %ld8 = load volatile i32, i32* @cond
  %is8 = icmp eq i32 %ld8, 8
  store volatile i1 %is8, i1* @cond2
  
  %or1 = or i1 %is1, %is2
  %or2 = or i1 %or1, %is3
  %or3 = or i1 %or2, %is4
  %or4 = or i1 %or3, %is5
  %or5 = or i1 %or4, %is6
  %or6 = or i1 %or5, %is7
  %or7 = or i1 %or6, %is8
  
  %result = zext i1 %or7 to i32
  
  ret i32 %result
}

define i32 @main(i32 %x)  {
entry: 
  store volatile i32 %x, i32* @cond
  %call1 = call i32 @init_func()
  ret i32 %call1
}
