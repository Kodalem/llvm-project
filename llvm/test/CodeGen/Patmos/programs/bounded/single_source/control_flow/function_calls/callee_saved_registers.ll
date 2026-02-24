; RUN: EXEC_ARGS="0=0 1=8 2=16 3=24 4=32 5=40"; \
; RUN: %test_execution
; END.
;//////////////////////////////////////////////////////////////////////////////////////////////////
;
; Tests that calling a function preserves the callee-saved registers
;
;//////////////////////////////////////////////////////////////////////////////////////////////////

@_1 = global i32 1

; We make this function require 32 registers such that it needs to save any
; callee saved register (r21-r28)
define void @use_all_registers(){
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
  %32 = load volatile i32, i32* @_1
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
  store volatile i32 %32, i32* @_1
  ret void
}

; We use a naked function to have full control over register use.
; Therefore, we also need to remember to save srb/sro since we are making a call.
define i32 @main(i32 %x) #0 {
entry:     
  ; We set all the callee-saved registers before calling the other function.
  ; We then use them after the call, to ensure their value hasn't changed.
  ; we don't check r29-r31 as they have special ABI uses
  call void asm sideeffect "
    sres 8
    mov $$r21 = $$r3
    mov $$r22 = $$r3
    mov $$r23 = $$r3
    mov $$r24 = $$r3
    mov $$r25 = $$r3
    mov $$r26 = $$r3
    mov $$r27 = $$r3
    mov $$r28 = $$r3
	mfs $$r9 = $$srb
    sws [0] = $$r9
	mfs $$r9 = $$sro
    sws [1] = $$r9
	", "~{r9},~{r10},~{r20},~{r21},~{r22},~{r23},~{r24},~{r25},~{r26},~{r27},~{r28},~{srb},~{sro}"
	()
  ; We split the assembly into 2 blocks to that function splitter can split freely
  ; without the assembly blocks being too big
  %0 = call i32 asm sideeffect "
	callnd $1
	lws $$r9 = [1]
	lws $$r10 = [0]
    mts $$sro = $$r9
    mts $$srb = $$r10
	li $0 = 0
	add $0 = $0, $$r21
	add $0 = $0, $$r22
	add $0 = $0, $$r23
	add $0 = $0, $$r24
	add $0 = $0, $$r25
	add $0 = $0, $$r26
	add $0 = $0, $$r27
	add $0 = $0, $$r28
	sfree 8
    ", "=r,r,~{r9},~{r10},~{r20},~{r21},~{r22},~{r23},~{r24},~{r25},~{r26},~{r27},~{r28},~{srb},~{sro}"
	(void ()* @use_all_registers)
  ret i32 %0
}
attributes #0 = { naked noinline }